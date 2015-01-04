#include "vm.h"
#include "core/ioservice_pool.h"
#include "core/thread_pool.h"
#include "port.h"
#include "core/concurrent_queue.h"

#include <boost/asio.hpp>
#include <string>
#include <shared_mutex>
#include <unordered_map>

#include <functional>
#include <boost/program_options.hpp>
#include <iostream>

namespace ultra {

/************************************************************************************
    vm::impl
 ***********************************************************************************/
struct vm::impl : core::ioservice_pool
{
    core::thread_pool<prio_scheduler> _pool;

    std::string _addr, _port;

    /// The signal_set is used to register for process
    /// termination notifications
    boost::asio::signal_set _signals;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor _acceptor;

    std::unordered_map<address, port *, address_hash> _space_map;

    impl(std::size_t num_threads, std::size_t num_ios,
         const std::string &address, const std::string &port);

    void start_accept();
    void handle_accept(std::shared_ptr<boost::asio::ip::tcp::socket> sock,
                       const boost::system::error_code &ec);
    void handle_stop();
};

vm::impl::impl(std::size_t num_threads, std::size_t num_ios,
               const std::string &address, const std::string &port)
    : core::ioservice_pool(num_ios), _pool(num_threads)
    , _addr(address), _port(port), _signals(next_io_service())
    , _acceptor(next_io_service())
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    _signals.add(SIGINT);
    _signals.add(SIGTERM);
#if defined(SIGQUIT)
    _signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
    _signals.async_wait(std::bind(&impl::handle_stop, this));
}

void vm::impl::start_accept()
{
    using namespace boost::asio;
    auto sock = std::make_shared<ip::tcp::socket>(next_io_service());
    _acceptor.async_accept(*sock,
        std::bind(&impl::handle_accept, this, sock, std::placeholders::_1));
}

void vm::impl::handle_accept(
        std::shared_ptr<boost::asio::ip::tcp::socket> sock,
        const boost::system::error_code &ec)
{
    using namespace boost::asio;

    if(!_acceptor.is_open())
        return;

    // TODO consume socket
    start_accept();
}

void vm::impl::handle_stop()
{
    _acceptor.close();
    stop();
}


/************************************************************************************
    vm
 ***********************************************************************************/
namespace po = boost::program_options;

static vm *s_instance;

vm::vm(int argc, const char **argv)
{
    s_instance = this;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
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
    std::string port = "80";

    if (vm.count("num-threads"))
        num_threads = vm["num-threads"].as<std::size_t>();

    if (vm.count("num-reactors"))
        num_ios = vm["num-reactors"].as<std::size_t>();

    if (vm.count("address"))
        addr = vm["address"].as<std::string>();

    if (vm.count("port"))
        port = vm["port"].as<std::string>();

    d = new impl(num_threads, num_ios, addr, port);

    if(argc) {
        using namespace boost::asio;

        // Open the acceptor with the option to reuse
        // the address (i.e. SO_REUSEADDR).
        ip::tcp::resolver resolver(d->_acceptor.get_io_service());
        ip::tcp::resolver::query query(addr, port);
        ip::tcp::endpoint endpoint = *resolver.resolve(query);
        d->_acceptor.open(endpoint.protocol());
        d->_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
        d->_acceptor.bind(endpoint);
        d->_acceptor.listen();

        d->_pool.reserve_thread();
        d->_pool.execute_callable(&impl::start_accept, d);
    }
}

vm::~vm()
{
    delete d;
}

/*static*/ vm *vm::instance()
{
    return s_instance;
}

void vm::register_port(port *n)
{
    d->_space_map[n->get_address()] = n;
}

void vm::unregister_port(port *n)
{
    d->_space_map.erase(n->get_address());
}

void vm::loop()
{
    d->next_io_service().poll();
}

void vm::post_message(scalar_message_ptr msg)
{
    port_ptr receiver = d->_space_map.at(msg->receiver())->shared_from_this();
    d->next_io_service().post(std::bind(&port::message, std::move(receiver), std::move(msg)));
//    d->_pool.execute_callable(&port::message, std::move(receiver), std::move(msg));
}

} // namespace ultra
