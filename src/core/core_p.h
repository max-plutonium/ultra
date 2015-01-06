#ifndef CORE_P_H
#define CORE_P_H

#include "../vm.h"
#include "core/ioservice_pool.h"
#include "core/thread_pool.h"
#include "core/concurrent_queue.h"
#include "node.h"
#include "message.h"
#include <boost/asio.hpp>

namespace ultra {

struct vm::impl : core::ioservice_pool
{
    core::thread_pool<core::prio_scheduler> _pool;

    std::string _addr, _port;

    /// The signal_set is used to register for process
    /// termination notifications
    boost::asio::signal_set _signals;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor _acceptor;

    static inline vm::impl *get() { return vm::instance()->d; }

    impl(std::size_t num_threads, std::size_t num_ios,
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

    explicit impl(port *p, ultra::openmode om)
        : std::stringbuf(static_cast<std::ios_base::openmode>(om))
        , _port(p), _om(om) { }

    virtual std::streamsize xsputn(const char_type *s, std::streamsize n)
    {
        std::string string(s, s + n);
        if(str().empty())
            str(string);
        else
            str(str() + '\n' + string);

        auto it = _receivers.begin();
        while(it != _receivers.end()) {
            auto curreceiver = *it;
            port_message msg(port_message::port_data, shared_from_this(),
                             curreceiver, _port->_time, string);
            vm::instance()->post_message(std::move(msg));
            ++it;
        }
        return n;
    }

    void on_message(const port_message &msg)
    {
        _port->_time.merge(msg.time());
        _port->_time.advance();

        switch(msg.type()) {

            case port_message::port_data:
                if(str().empty())
                    str(msg.data());
                else
                    str(str() + '\n' + msg.data());
                break;

            case port_message::connect_sender:
                connect_sender(msg.sender());
                break;

            case port_message::disconnect_sender:
                disconnect_sender(msg.sender());
                break;

            case port_message::disconnect_receiver:
                disconnect_receiver(msg.sender());
                break;

            default:
                break;
        }
    }

    bool connect_sender(const std::shared_ptr<port::impl> &asender)
    {
        assert(asender);
        auto it = std::find_if(_senders.cbegin(),
            _senders.cend(), [&asender](const auto &entry) {
                return asender == entry;
            });

        if(it != _senders.cend())
            return false;

        _senders.emplace_back(asender);
        return true;
    }

    bool connect_receiver(const std::shared_ptr<port::impl> &areceiver)
    {
        assert(areceiver);
        auto it = std::find_if(_receivers.cbegin(),
            _receivers.cend(), [&areceiver](const auto &entry) {
                return areceiver == entry;
            });

        if(it != _receivers.cend())
            return false;

        _receivers.emplace_back(areceiver);
        return true;
    }

    void disconnect_sender(const std::shared_ptr<port::impl> &asender)
    {
        assert(asender);
        auto it = std::find_if(_senders.cbegin(),
            _senders.cend(), [&asender](const auto &entry) {
                return asender == entry;
            });

        _senders.erase(it);
    }
    void disconnect_receiver(const std::shared_ptr<port::impl> &areceiver)
    {
        assert(areceiver);
        auto it = std::find_if(_receivers.cbegin(),
            _receivers.cend(), [&areceiver](const auto &entry) {
                return areceiver == entry;
            });

        _receivers.erase(it);
    }

    void disconnect_all_senders()
    {
        for(const auto &sender : _senders) {
            _port->_time.advance();
            port_message msg(port_message::disconnect_receiver,
                shared_from_this(), sender, _port->_time);
            vm::instance()->post_message(std::move(msg));
        }

        _senders.clear();
    }
    void disconnect_all_receivers()
    {
        for(const auto &receiver : _receivers) {
            _port->_time.advance();
            port_message msg(port_message::disconnect_sender,
                shared_from_this(), receiver, _port->_time);
            vm::instance()->post_message(std::move(msg));
        }

        _receivers.clear();
    }
};

namespace core {

} // namespace core

} // namespace ultra

#endif // CORE_P_H

