#include "message.h"
#include "msg.pb.h"

namespace ultra {

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

//std::ostream &operator<<(std::ostream &o, const message &msg)
//{
//    internal::scalar_message int_message;
//    int_message.mutable_time()->set_counter(msg._time.time());
//    auto *s = int_message.mutable_sender();
//    s->set_x(msg._sender.x());
//    s->set_y(msg._sender.y());
//    s->set_z(msg._sender.z());
//    auto *r = int_message.mutable_receiver();
//    r->set_x(msg._receiver.x());
//    r->set_y(msg._receiver.y());
//    r->set_z(msg._receiver.z());
//    int_message.set_type(
//        static_cast<internal::scalar_message::msg_type>(msg._type));
//    int_message.set_data(msg._data);
//    int_message.SerializeToOstream(&o);
//    return o;
//}

//std::istream &operator>>(std::istream &i, message &msg)
//{
//    internal::scalar_message int_message;
//    int_message.ParseFromIstream(&i);
//    msg._time = scalar_time(int_message.time().counter());
//    auto &s = int_message.sender();
//    msg._sender = address(s.x(), s.y(), s.z());
//    auto &r = int_message.receiver();
//    msg._receiver = address(r.x(), r.y(), r.z());
//    msg._type = static_cast<message::msg_type>(int_message.type());
//    msg._data = int_message.data();
//    return i;
//}

} // namespace ultra
