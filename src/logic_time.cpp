#include "logic_time.h"
#include "msg.pb.h"

namespace ultra {

scalar_time::scalar_time(std::size_t t)
    : _time(t)
{
}

bool scalar_time::advance()
{
    ++_time;
    return true;
}

std::size_t scalar_time::time() const
{
    return _time;
}

void scalar_time::merge(const scalar_time &other)
{
    _time = std::max(_time, other._time);
}

bool scalar_time::operator==(const scalar_time &o) const
{
    return _time == o._time;
}

bool scalar_time::operator!=(const scalar_time &o) const
{
    return !operator==(o);
}

std::ostream &operator<<(std::ostream &o, const ultra::scalar_time &msg)
{
    internal::scalar_time int_time;
    int_time.set_counter(msg._time);
    int_time.SerializeToOstream(&o);
    return o;
}

std::istream &operator>>(std::istream &i, ultra::scalar_time &msg)
{
    internal::scalar_time int_time;
    int_time.ParseFromIstream(&i);
    msg._time = int_time.counter();
    return i;
}

} // namespace ultra
