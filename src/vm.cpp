#include <boost/program_options.hpp>
#include <boost/asio/spawn.hpp>
#include <functional>

#include <cds/init.h>

#include "core/action.h"
#include "core/core_p.h"
#include "core/network_session.h"
#include "messages.h"
#include "port.h"

namespace ultra {

/************************************************************************************
    vm::impl
 ***********************************************************************************/
vm::impl::impl(int cluster, std::size_t num_threads, std::size_t num_network_threads,
               std::size_t num_ios, const std::string &address, const std::string &port)
    : core::ioservice_pool(num_ios), _cluster(cluster)
    , _work_threads(schedule_type::prio, num_threads), _stop_all(false)
    , _addr(address), _port(port), _signals(*next_io_service())
    , _acceptor(*next_io_service())
{
    using namespace boost::asio;

    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    _signals.add(SIGINT);
    _signals.add(SIGABRT);
    _signals.add(SIGTERM);
    _signals.add(SIGQUIT);
    _signals.async_wait(std::bind(&impl::handle_stop, this));

    // Open the acceptor with the option to reuse
    // the address (i.e. SO_REUSEADDR).
    ip::tcp::resolver resolver(_acceptor.get_io_service());
    ip::tcp::resolver::query query(ip::tcp::v4(), port);
    ip::tcp::endpoint endpoint = *resolver.resolve(query);
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();

    boost::asio::spawn(_acceptor.get_io_service(),
        [this](boost::asio::yield_context yield)
        {
            for (;;) {
                boost::system::error_code ec;
                auto sock = std::make_shared<ip::tcp::socket>(*next_io_service());
                _acceptor.async_accept(*sock, yield[ec]);
                if (!ec)
                    std::make_shared<core::network_session>(std::move(sock))->start();
            }
        });

    for(std::size_t i = 0; i < num_network_threads; ++i) {
        _net_threads.create_thread([this]() {
            do {
                next_io_service()->run_one();
            } while(!_stop_all);
        });
    }
}

vm::impl::~impl()
{
    handle_stop();
    _net_threads.join_all();
    _work_threads.wait_for_done();
}

void vm::impl::handle_stop()
{
    _acceptor.close();
    _stop_all = true;
    stop();
}


/************************************************************************************
    vm
 ***********************************************************************************/
namespace po = boost::program_options;

vm *g_instance;

vm::vm(int argc, const char **argv)
{
    g_instance = this;

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("cluster,c", po::value<int>(), "number of cluster")
            ("num-threads,t", po::value<std::size_t>(), "number of threads")
            ("num-network-threads,n", po::value<std::size_t>(), "number of network threads")
            ("num-reactors,r", po::value<std::size_t>(), "number of reactors")
            ("address,a", po::value<std::string>(), "ip address")
            ("port,p", po::value<std::string>(), "port for listening")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
        std::cout << desc << "\n";

    std::size_t num_threads = 1;
    std::size_t num_network_threads = 1;
    std::size_t num_ios = 1;
    std::string addr = "127.0.0.1";
    std::string port = "55799";
    int cluster = 0;

    if (vm.count("num-threads"))
        num_threads = vm["num-threads"].as<std::size_t>();

    if (vm.count("num-network-threads"))
        num_network_threads = vm["num-network-threads"].as<std::size_t>();

    if (vm.count("num-reactors"))
        num_ios = vm["num-reactors"].as<std::size_t>();

    if (vm.count("address"))
        addr = vm["address"].as<std::string>();

    if (vm.count("port"))
        port = vm["port"].as<std::string>();

    if (vm.count("cluster"))
        cluster = vm["cluster"].as<int>();

    cds::Initialize();

    d = new impl(cluster, num_threads, num_network_threads, num_ios, addr, port);
}

vm::~vm()
{
    delete d;

    cds::Terminate();
}

/*static*/ vm *vm::instance()
{
    return g_instance;
}

void vm::wait_for_done()
{
    d->stop();
    d->_net_threads.join_all();
    d->_work_threads.wait_for_done();
}

void vm::loop()
{
    auto res = d->next_io_service()->poll();
    return;
}

void vm::post_message(port_message msg)
{
    d->next_io_service()->post(core::make_action(
        &port::impl::on_message, msg.receiver(), std::move(msg)));
}

} // namespace ultra
