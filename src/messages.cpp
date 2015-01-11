#include "messages.h"
#include "msg.pb.h"
#include "core/core_p.h"

namespace ultra {

/************************************************************************************
    request
 ***********************************************************************************/
request::request(request::command_type type, const std::string &data)
    : _type(type), _data(data)
{
}

request::command_type request::type() const
{
    return _type;
}

std::string request::data() const
{
    return _data;
}

std::ostream &operator<<(std::ostream &o, const request &msg)
{
    internal::request int_request;
    int_request.set_type(static_cast<internal::request::command_type>(msg._type));
    int_request.set_data(msg._data);
    int_request.SerializeToOstream(&o);
    return o;
}

std::istream &operator>>(std::istream &i, request &msg)
{
    internal::request int_request;
    int_request.ParseFromIstream(&i);
    msg._type = static_cast<request::command_type>(int_request.type());
    msg._data = int_request.data();
    return i;
}


/************************************************************************************
    reply
 ***********************************************************************************/
reply::reply(reply::command_type type, const std::string &data)
    : _type(type), _data(data)
{
}

reply::command_type reply::type() const
{
    return _type;
}

std::string reply::data() const
{
    return _data;
}

std::ostream &operator<<(std::ostream &o, const reply &msg)
{
    internal::reply int_reply;
    int_reply.set_type(static_cast<internal::reply::command_type>(msg._type));
    int_reply.set_data(msg._data);
    int_reply.SerializeToOstream(&o);
    return o;
}

std::istream &operator>>(std::istream &i, reply &msg)
{
    internal::reply int_reply;
    int_reply.ParseFromIstream(&i);
    msg._type = static_cast<reply::command_type>(int_reply.type());
    msg._data = int_reply.data();
    return i;
}


/************************************************************************************
    port_message
 ***********************************************************************************/
port_message::port_message(msg_type type,
    std::shared_ptr<port::impl> sender, std::shared_ptr<port::impl> receiver,
    scalar_time t /*= scalar_time()*/, const std::string &data /*= ""*/)
    : _time(std::move(t)), _sender(sender)
    , _receiver(receiver), _data(data), _type(type)
{
}

scalar_time port_message::time() const
{
    return _time;
}

std::shared_ptr<port::impl> port_message::sender() const
{
    return _sender;
}

std::shared_ptr<port::impl> port_message::receiver() const
{
    return _receiver;
}

port_message::msg_type port_message::type() const
{
    return _type;
}

std::string port_message::data() const
{
    return _data;
}

bool port_message::operator==(const port_message &o) const
{
    return (_time == o._time) && (_sender == o._sender)
            && (_receiver == o._receiver) && (_data == o._data)
            && (_type == o._type);
}

bool port_message::operator!=(const port_message &o) const
{
    return !operator==(o);
}

} // namespace ultra
