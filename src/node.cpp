#include "node.h"
#include "vm.h"
#include <cassert>
#include <sstream>

namespace ultra {

/************************************************************************************
    node
 ***********************************************************************************/
node::node(address a)
    : _addr(a), _senders(nullptr), _receivers(nullptr)
{
}

node::~node()
{
    disconnect_all_senders();
    disconnect_all_receivers();
    assert(!_senders && !_receivers);
}

address node::get_address() const
{
    return _addr;
}

scalar_time node::time() const
{
    return _time;
}

bool node::connect(const node_ptr &areceiver)
{
    assert(areceiver);
    edge *cur = _receivers;

    if(cur) {
        node_ptr curreceiver = cur->_receiver.lock();
        edge *before_cur = nullptr;

        while(curreceiver < areceiver) {
            before_cur = cur;
            cur = cur->_down;
            if(!cur) break;
            curreceiver = cur->_receiver.lock();
        }
        if(cur && curreceiver == areceiver)
            return false;
        edge *c = new edge(shared_from_this(), areceiver);
        c->_down = cur;
        if(before_cur)
            before_cur->_down = c;
        else
            _receivers = c;
        cur = c;
    }
    else
        _receivers = cur = new edge(shared_from_this(), areceiver);

    _time.advance();

    std::string str_ptr;
    std::stringstream ss; ss << cur; ss >> str_ptr;
    auto msg = std::make_shared<scalar_message>(
        scalar_message::connect_sender, _addr, areceiver->_addr, _time, str_ptr);
    vm::instance()->post_message(std::move(msg));
    return true;
}

bool node::disconnect(const node_ptr &areceiver)
{
    assert(areceiver);
    edge *cur = _receivers;

    if(cur)
    {
        node_ptr curreceiver = cur->_receiver.lock();
        edge *before_cur = nullptr;

        while(curreceiver < areceiver)
        {
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

void node::message(const scalar_message_ptr &msg)
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

        case scalar_message::connect_sender:
            connect_sender(reinterpret_cast<edge *>(data_as_ptr));
            break;

        case scalar_message::disconnect_sender:
            disconnect_sender(reinterpret_cast<node *>(data_as_ptr)
                              ->shared_from_this());
            break;

        default:
            break;
    }
}

void node::post_message(scalar_message::msg_type type,
                        address addr, const std::string &data)
{
    _time.advance();
    auto msg = std::make_shared<scalar_message>(type, _addr, addr, _time, data);

    edge *cur = _receivers, *before_cur = nullptr;

    while(cur) {
        node_ptr curreceiver = cur->_receiver.lock();
        if(ULTRA_EXPECT(curreceiver, true)) {
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

void node::connect_sender(edge *c)
{
    assert(c);
    edge *cur = _senders;
    node_ptr sender = c->_sender.lock();
    assert(sender);

    if(cur) {
        node_ptr cursender = cur->_sender.lock();
        edge *before_cur = nullptr;

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

void node::disconnect_sender(const node_ptr &asender)
{
    assert(asender);
    edge *cur = _senders;

    if(cur)
    {
        node_ptr cursender = cur->_sender.lock();
        edge *before_cur = nullptr;

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

void node::disconnect_all_senders()
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

void node::disconnect_all_receivers()
{
    while(_receivers) {
        node_ptr curreceiver = _receivers->_receiver.lock();

        if(!curreceiver) {
            auto old_cur = _receivers;
            _receivers = _receivers->_down;
            delete old_cur;

        } else {
            _time.advance();

            std::string str_ptr;
            std::stringstream ss; ss << this; ss >> str_ptr;
            auto msg = std::make_shared<scalar_message>(
                scalar_message::disconnect_sender, _addr, curreceiver->_addr, _time, str_ptr);
            vm::instance()->post_message(std::move(msg));
            _receivers = _receivers->_down;
        }
    }
}


/************************************************************************************
    edge
 ***********************************************************************************/
edge::edge(node_ptr sender, node_ptr receiver)
    : _sender(sender), _receiver(receiver)
    , _next(nullptr), _down(nullptr)
{
}

} // namespace ultra
