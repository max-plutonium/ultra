#include "port.h"
#include "vm.h"
#include <cassert>
#include <algorithm>

namespace ultra {

class port::notifier : public std::stringbuf
{
public:
    port *_port;

    explicit notifier(port *p) : _port(p) { }

    virtual std::streamsize xsputn(const char_type *s, std::streamsize n)
    {
        std::string string(s, s + n);
        _port->post_message(scalar_message::port_data, string);
        return n;
    }
};


/************************************************************************************
    port
 ***********************************************************************************/
port::port(address a, ultra::openmode om)
    : std::stringstream(static_cast<std::ios_base::openmode>(om))
    , _addr(a), _time(), _om(om), _notifier(new notifier(this))
{
    vm::instance()->register_port(this);
    std::iostream::rdbuf(_notifier);
    this->init(_notifier);
}

port::~port()
{
    disconnect_all_senders();
    disconnect_all_receivers();
    vm::instance()->unregister_port(this);
    delete _notifier;
}

address port::get_address() const
{
    return _addr;
}

scalar_time port::time() const
{
    return _time;
}

openmode port::open_mode() const
{
    return _om;
}

bool port::connect(const port_ptr &areceiver)
{
    if(!connect_receiver(areceiver))
        return false;

    _time.advance();
    std::string str_ptr;
    std::stringstream ss; ss << this; ss >> str_ptr;
    auto msg = std::make_shared<scalar_message>(
        scalar_message::connect_sender, _addr, areceiver->_addr, _time, str_ptr);
    vm::instance()->post_message(std::move(msg));
    return true;
}

void port::disconnect(const port_ptr &areceiver)
{
    disconnect_receiver(areceiver);

    _time.advance();
    std::string str_ptr;
    std::stringstream ss; ss << this; ss >> str_ptr;
    auto msg = std::make_shared<scalar_message>(
        scalar_message::disconnect_sender, _addr, areceiver->_addr, _time, str_ptr);
    vm::instance()->post_message(std::move(msg));
}

void port::message(const scalar_message_ptr &msg)
{
    _time.merge(msg->time());
    _time.advance();

    std::intptr_t data_as_ptr = 0;

    if(!msg->data().substr(0, 2).compare("0x")) {
        std::stringstream ss;
        ss << msg->data();
        ss >> std::hex >> data_as_ptr;
    }

    switch(msg->type()) {

        case scalar_message::port_data:
            if(_notifier->str().empty())
                _notifier->str(msg->data());
            else
                _notifier->str(_notifier->str() + '\n' + msg->data());
            break;

        case scalar_message::connect_sender:
            connect_sender(reinterpret_cast<port *>(data_as_ptr)->shared_from_this());
            break;

        case scalar_message::disconnect_sender:
            disconnect_sender(reinterpret_cast<port *>(data_as_ptr)->shared_from_this());
            break;

        case scalar_message::disconnect_receiver:
            disconnect_receiver(reinterpret_cast<port *>(data_as_ptr)->shared_from_this());
            break;

        default:
            break;
    }
}

void port::post_message(scalar_message::msg_type type, const std::string &data)
{
    _time.advance();

    auto it = _receivers.begin();
    while(it != _receivers.end()) {
        port_ptr curreceiver = it->lock();
        if(ULTRA_EXPECT(curreceiver, true)) {
            auto msg = std::make_shared<scalar_message>(type, _addr,
                curreceiver->get_address(), _time, data);
            vm::instance()->post_message(msg);
            ++it;

        } else
            _receivers.erase(it);
    }
}

bool port::connect_sender(const std::shared_ptr<port> &asender)
{
    assert(asender);
    auto it = std::find_if(_senders.cbegin(),
        _senders.cend(), [&asender](const auto &entry) {
            return asender == entry.lock();
        });

    if(it != _senders.cend())
        return false;

    _senders.emplace_back(asender);
    return true;
}

bool port::connect_receiver(const std::shared_ptr<port> &areceiver)
{
    assert(areceiver);
    auto it = std::find_if(_receivers.cbegin(),
        _receivers.cend(), [&areceiver](const auto &entry) {
            return areceiver == entry.lock();
        });

    if(it != _receivers.cend())
        return false;

    _receivers.emplace_back(areceiver);
    return true;
}

void port::disconnect_sender(const std::shared_ptr<port> &asender)
{
    assert(asender);
    auto it = std::find_if(_senders.cbegin(),
        _senders.cend(), [&asender](const auto &entry) {
            return asender == entry.lock();
        });

    _senders.erase(it);
}

void port::disconnect_receiver(const std::shared_ptr<port> &areceiver)
{
    assert(areceiver);
    auto it = std::find_if(_receivers.cbegin(),
        _receivers.cend(), [&areceiver](const auto &entry) {
            return areceiver == entry.lock();
        });

    _receivers.erase(it);
}

void port::disconnect_all_senders()
{
    for(std::weak_ptr<port> &sender : _senders) {
        auto strong_sender = sender.lock();
        if(!strong_sender)
            continue;
        _time.advance();
        std::string str_ptr;
        std::stringstream ss; ss << this; ss >> str_ptr;
        auto msg = std::make_shared<scalar_message>(
            scalar_message::disconnect_receiver, _addr, strong_sender->_addr, _time, str_ptr);
        vm::instance()->post_message(std::move(msg));
    }

    _senders.clear();
}

void port::disconnect_all_receivers()
{
    for(const std::weak_ptr<port> &receiver : _receivers) {
        auto strong_receiver = receiver.lock();
        if(!strong_receiver)
            continue;
        _time.advance();
        std::string str_ptr;
        std::stringstream ss; ss << this; ss >> str_ptr;
        auto msg = std::make_shared<scalar_message>(
            scalar_message::disconnect_sender, _addr, strong_receiver->_addr, _time, str_ptr);
        vm::instance()->post_message(std::move(msg));
    }

    _receivers.clear();
}

} // namespace ultra
