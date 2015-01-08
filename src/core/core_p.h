#ifndef CORE_P_H
#define CORE_P_H

#include <boost/asio.hpp>

#include "../vm.h"
#include "core/ioservice_pool.h"
#include "core/thread_pool.h"
#include "node.h"
#include "message.h"
#include "system.h"

namespace ultra {

struct vm::impl : core::ioservice_pool
{
    int _cluster;

    core::thread_pool _pool;
    sched_ptr _scheduler;

    std::string _addr, _port;

    /// The signal_set is used to register for process
    /// termination notifications
    boost::asio::signal_set _signals;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor _acceptor;

    static inline vm::impl *get() { return vm::instance()->d; }

    impl(int cluster, std::size_t num_threads, std::size_t num_ios,
         const std::string &address, const std::string &port);

    void start_accept();
    void handle_accept(std::shared_ptr<boost::asio::ip::tcp::socket> sock,
                       const boost::system::error_code &ec);
    void handle_stop();
};

extern vm *g_instance;

class port::impl : public std::stringbuf
        , public std::enable_shared_from_this<port::impl>
{
public:
    port *_port;
    ultra::openmode _om;
    std::deque<std::shared_ptr<port::impl>> _senders, _receivers;

    explicit impl(port *p, ultra::openmode om);

    virtual std::streamsize xsputn(const char_type *s, std::streamsize n);

    void on_message(const port_message &msg);
    bool connect_sender(const std::shared_ptr<port::impl> &asender);
    bool connect_receiver(const std::shared_ptr<port::impl> &areceiver);

    void disconnect_sender(const std::shared_ptr<port::impl> &asender);
    void disconnect_receiver(const std::shared_ptr<port::impl> &areceiver);
    void disconnect_all_senders();
    void disconnect_all_receivers();
};

} // namespace ultra

#endif // CORE_P_H

