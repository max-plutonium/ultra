#include "core/core_p.h"
#include "port.h"
#include "message.h"
#include "core/action.h"

#include <functional>
#include <boost/program_options.hpp>
#include <iostream>

namespace ultra {

/************************************************************************************
    vm::impl
 ***********************************************************************************/
vm::impl::impl(int cluster, std::size_t num_threads, std::size_t num_ios,
               const std::string &address, const std::string &port)
    : core::ioservice_pool(num_ios), _cluster(cluster)
    , _pool(schedule_type::prio, num_threads)
    , _scheduler(_pool.sched()), _addr(address), _port(port)
    , _signals(), _acceptor()
{
}

vm::impl::~impl()
{
    _pool.wait_for_done();
}

void vm::impl::start_accept(std::shared_ptr<boost::asio::io_service> accept_ios)
{
    using namespace boost::asio;
    auto sock = std::make_shared<ip::tcp::socket>(*accept_ios);
    _acceptor->async_accept(*sock, std::bind(&impl::handle_accept, this,
            std::move(accept_ios), sock, std::placeholders::_1));
}

void vm::impl::handle_accept(std::shared_ptr<boost::asio::io_service> accept_ios,
        std::shared_ptr<boost::asio::ip::tcp::socket> sock,
        const boost::system::error_code &ec)
{
    using namespace boost::asio;

    if(!_acceptor->is_open())
        return;

    // TODO consume socket
    start_accept(std::move(accept_ios));
}

void vm::impl::handle_stop()
{
    _acceptor->close();
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
    std::size_t num_ios = 1;
    std::string addr = "127.0.0.1";
    std::string port = "55004";
    int cluster = 0;

    if (vm.count("num-threads"))
        num_threads = vm["num-threads"].as<std::size_t>();

    if (vm.count("num-reactors"))
        num_ios = vm["num-reactors"].as<std::size_t>();

    if (vm.count("address"))
        addr = vm["address"].as<std::string>();

    if (vm.count("port"))
        port = vm["port"].as<std::string>();

    if (vm.count("cluster"))
        cluster = vm["cluster"].as<int>();

    d = new impl(cluster, num_threads, num_ios, addr, port);

    if(!argc)
        return;

    using namespace boost::asio;
    auto accept_service = std::make_shared<io_service>();
    d->_signals = std::make_unique<boost::asio::signal_set>(*accept_service);
    d->_acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*accept_service);

    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    d->_signals->add(SIGINT);
    d->_signals->add(SIGTERM);
    d->_signals->add(SIGQUIT);
    d->_signals->async_wait(std::bind(&impl::handle_stop, d));

    // Open the acceptor with the option to reuse
    // the address (i.e. SO_REUSEADDR).
    ip::tcp::resolver resolver(*accept_service);
    ip::tcp::resolver::query query(addr, port);
    ip::tcp::endpoint endpoint = *resolver.resolve(query);
    d->_acceptor->open(endpoint.protocol());
    d->_acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
    d->_acceptor->bind(endpoint);
    d->_acceptor->listen();

    d->_pool.reserve_thread();
    accept_service->post(core::make_action(&impl::start_accept, d, accept_service));
    std::size_t (io_service::*ios_run)() = &io_service::run;
    d->_pool.execute_callable(1, core::make_action(ios_run, std::move(accept_service)));
}

vm::~vm()
{
    delete d;
}

/*static*/ vm *vm::instance()
{
    return g_instance;
}

void vm::loop()
{
    d->next_io_service()->run();
}

void vm::post_message(port_message msg)
{
    d->_pool.execute_callable(1, core::make_action(
            &port::impl::on_message, msg.receiver(), std::move(msg)));
}

} // namespace ultra
