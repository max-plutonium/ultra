#ifndef CORE_P_H
#define CORE_P_H

#include <boost/asio.hpp>
#include <boost/thread.hpp> // thread group

#include <cds/gc/hp.h>

#include "../vm.h"

#include "core/ioservice_pool.h"
#include "core/thread_pool.h"
#include "messages.h"
#include "system.h"

namespace ultra {

struct timed_task : std::enable_shared_from_this<timed_task>
{
    task_ptr _task;
    boost::asio::deadline_timer _timer;
    execution_service *_exec;

    timed_task(const task_ptr &t, execution_service *executor);
    timed_task(const task_ptr &t,
               const std::shared_ptr<boost::asio::io_service> &ios,
               execution_service *executor);
    void start(std::size_t delay_msecs = 0, std::size_t period_msecs = 0);
};

struct vm::impl : public core::ioservice_pool
{
    int _cluster;
    core::thread_pool _work_threads;
    boost::thread_group _net_threads;
    std::atomic_bool _stop_all;
    std::string _addr, _port;

    /// The signal_set is used to register for process
    /// termination notifications
    boost::asio::signal_set _signals;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor _acceptor;

    cds::gc::HP _gc;

    impl(int cluster, std::size_t num_threads, std::size_t num_network_threads,
         std::size_t num_ios, const std::string &address, const std::string &port);

    ~impl();

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

    virtual std::streamsize xsputn(const char_type *s, std::streamsize n) override;

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
