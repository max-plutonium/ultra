#include "core_p.h"

namespace ultra {

port::impl::impl(port *p, ultra::openmode om)
    : std::stringbuf(static_cast<std::ios_base::openmode>(om))
    , _port(p), _om(om) { }

std::streamsize port::impl::xsputn(const char_type *s, std::streamsize n)
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

void port::impl::on_message(const port_message &msg)
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

bool port::impl::connect_sender(const std::shared_ptr<port::impl> &asender)
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

bool port::impl::connect_receiver(const std::shared_ptr<port::impl> &areceiver)
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

void port::impl::disconnect_sender(const std::shared_ptr<port::impl> &asender)
{
    assert(asender);
    auto it = std::find_if(_senders.cbegin(),
        _senders.cend(), [&asender](const auto &entry) {
            return asender == entry;
        });

    _senders.erase(it);
}

void port::impl::disconnect_receiver(const std::shared_ptr<port::impl> &areceiver)
{
    assert(areceiver);
    auto it = std::find_if(_receivers.cbegin(),
        _receivers.cend(), [&areceiver](const auto &entry) {
            return areceiver == entry;
        });

    _receivers.erase(it);
}

void port::impl::disconnect_all_senders()
{
    for(const auto &sender : _senders) {
        _port->_time.advance();
        port_message msg(port_message::disconnect_receiver,
                         shared_from_this(), sender, _port->_time);
        vm::instance()->post_message(std::move(msg));
    }

    _senders.clear();
}

void port::impl::disconnect_all_receivers()
{
    for(const auto &receiver : _receivers) {
        _port->_time.advance();
        port_message msg(port_message::disconnect_sender,
                         shared_from_this(), receiver, _port->_time);
        vm::instance()->post_message(std::move(msg));
    }

    _receivers.clear();
}

} // namespace ultra
