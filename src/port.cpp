#include "port.h"
#include "vm.h"
#include <cassert>

namespace ultra {

class port::connection
{
    std::weak_ptr<port> _sender, _receiver;
    connection *_next, *_down;

public:
    connection(port_ptr sender, port_ptr receiver);
    friend class port;
};

class port::notifier : public std::stringbuf
{
    port *_port;

public:
    explicit notifier(port *p) : _port(p) { }

    virtual std::streamsize xsputn(const char_type *s, std::streamsize n)
    {
        std::string str(s, s + n);
        _port->post_message(scalar_message::port_data, str);
        return n;
    }
};


/************************************************************************************
    port::connection
 ***********************************************************************************/
port::connection::connection(port_ptr sender, port_ptr receiver)
    : _sender(sender), _receiver(receiver)
    , _next(nullptr), _down(nullptr)
{
}


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
    assert(!_senders && !_receivers);
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
    assert(areceiver);
    connection *cur = _receivers;

    if(cur) {
        port_ptr curreceiver = cur->_receiver.lock();
        connection *before_cur = nullptr;

        while(curreceiver < areceiver) {
            before_cur = cur;
            cur = cur->_down;
            if(!cur) break;
            curreceiver = cur->_receiver.lock();
        }
        if(cur && curreceiver == areceiver)
            return false;
        connection *c = new connection(shared_from_this(), areceiver);
        c->_down = cur;
        if(before_cur)
            before_cur->_down = c;
        else
            _receivers = c;
        cur = c;
    }
    else
        _receivers = cur = new connection(shared_from_this(), areceiver);

    _time.advance();

    std::string str_ptr;
    std::stringstream ss; ss << cur; ss >> str_ptr;
    auto msg = std::make_shared<scalar_message>(
        scalar_message::connect_sender, _addr, areceiver->_addr, _time, str_ptr);
    vm::instance()->post_message(std::move(msg));
    return true;
}

bool port::disconnect(const port_ptr &areceiver)
{
    assert(areceiver);
    connection *cur = _receivers;

    if(cur)
    {
        port_ptr curreceiver = cur->_receiver.lock();
        connection *before_cur = nullptr;

        while(curreceiver < areceiver) {
            before_cur = cur;
            cur = cur->_down;
            if(!cur) break;
            curreceiver = cur->_receiver.lock();
        }
        if(cur && curreceiver == areceiver)
        {
            if(before_cur)
                before_cur->_down = cur->_down;
            else
                _receivers = cur->_down;

            if(!cur->_receiver.expired()) {
                _time.advance();
                std::string str_ptr;
                std::stringstream ss; ss << this; ss >> str_ptr;
                auto msg = std::make_shared<scalar_message>(
                    scalar_message::disconnect_sender, _addr, areceiver->_addr, _time, str_ptr);
                vm::instance()->post_message(std::move(msg));
            }
            else
                delete cur;

            return true;
        }
    }
    return false;
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
            connect_sender(reinterpret_cast<connection *>(data_as_ptr));
            break;

        case scalar_message::disconnect_sender:
            disconnect_sender(reinterpret_cast<port *>(data_as_ptr)
                              ->shared_from_this());
            break;

        default:
            break;
    }
}

void port::post_message(scalar_message::msg_type type, const std::string &data)
{
    _time.advance();

    connection *cur = _receivers, *before_cur = nullptr;

    while(cur) {
        port_ptr curreceiver = cur->_receiver.lock();
        if(ULTRA_EXPECT(curreceiver, true)) {
            auto msg = std::make_shared<scalar_message>(type, _addr,
                curreceiver->get_address(), _time, data);
            vm::instance()->post_message(msg);
            before_cur = cur;
            cur = cur->_down;

        } else {
            if(before_cur)
                before_cur->_down = cur->_down;
            else
                _receivers = cur->_down;
            auto old_cur = cur;
            cur = cur->_down;
            delete old_cur;
        }
    }
}

void port::connect_sender(connection *c)
{
    assert(c);
    connection *cur = _senders;
    port_ptr sender = c->_sender.lock();
    assert(sender);

    if(cur) {
        port_ptr cursender = cur->_sender.lock();
        connection *before_cur = nullptr;

        while(cursender < sender)
        {
            before_cur = cur;
            cur = cur->_next;
            if(!cur) break;
            cursender = cur->_sender.lock();
        }
        if(cursender != sender)
        {
            c->_next = cur;
            if(before_cur)
                before_cur->_next = c;
            else
                _senders = c;
        }
    }
    else
        _senders = c;
}

void port::disconnect_sender(const port_ptr &asender)
{
    assert(asender);
    connection *cur = _senders;

    if(cur)
    {
        port_ptr cursender = cur->_sender.lock();
        connection *before_cur = nullptr;

        while(cursender < asender) {
            before_cur = cur;
            cur = cur->_next;
            if(!cur) break;
            cursender = cur->_sender.lock();
        }

        if(cur && cursender == asender)
        {
            if(before_cur)
                before_cur->_next = cur->_next;
            else
                _senders = cur->_next;

            if(!cur->_receiver.expired()) {
                _time.advance();
                std::string str_ptr;
                std::stringstream ss; ss << this; ss >> str_ptr;
                auto msg = std::make_shared<scalar_message>(
                    scalar_message::disconnect_sender, _addr, asender->_addr, _time, str_ptr);
                vm::instance()->post_message(std::move(msg));
            }
            else
                delete cur;
        }
    }
}

void port::disconnect_all_senders()
{
    while(_senders) {
        if(_senders->_receiver.expired()) {
            auto old_cur = _senders;
            _senders = _senders->_next;
            delete old_cur;
            continue;
        }
        _senders = _senders->_next;
    }
}

void port::disconnect_all_receivers()
{
    while(_receivers) {
        port_ptr curreceiver = _receivers->_receiver.lock();

        if(!curreceiver) {
            auto old_cur = _receivers;
            _receivers = _receivers->_down;
            delete old_cur;

        } else {
            _time.advance();

//            std::string str_ptr;
//            std::stringstream ss;
//            ss << this;
//            ss >> str_ptr;
//            auto msg = std::make_shared<scalar_message>(
//                scalar_message::disconnect_sender, _addr, curreceiver->_addr, _time, str_ptr);
//            vm::instance()->post_message(std::move(msg));
            _receivers = _receivers->_down;
        }
    }
}

} // namespace ultra
