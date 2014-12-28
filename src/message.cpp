#include "message.h"
#include "msg.pb.h"

namespace ultra {

scalar_message::scalar_message(scalar_time t, address a, const char *data)
    : _time(std::move(t)), _sender_address(a), _data(data)
{
}

bool scalar_message::operator==(const scalar_message &o) const
{
    return (_time == o._time) && (_sender_address == o._sender_address)
            && (_data == o._data);
}

bool scalar_message::operator!=(const scalar_message &o) const
{
    return !operator==(o);
}

std::ostream &operator<<(std::ostream &o, const scalar_message &msg)
{
    internal::scalar_message int_message;
    int_message.mutable_time()->set_counter(msg._time.time());
    auto *s = int_message.mutable_sender();
    s->set_x(msg._sender_address.x());
    s->set_y(msg._sender_address.y());
    s->set_z(msg._sender_address.z());
    int_message.set_data(msg._data);
    int_message.SerializeToOstream(&o);
    return o;
}

std::istream &operator>>(std::istream &i, scalar_message &msg)
{
    internal::scalar_message int_message;
    int_message.ParseFromIstream(&i);
    msg._time = scalar_time(int_message.time().counter());
    auto &s = int_message.sender();
    msg._sender_address = address(s.x(), s.y(), s.z());
    msg._data = int_message.data();
    return i;
}

} // namespace ultra
