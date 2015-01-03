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

bool scalar_time::operator<=(const scalar_time &o) const
{
    return _time <= o._time;
}

bool scalar_time::operator<(const scalar_time &o) const
{
    return _time < o._time;
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


/************************************************************************************
    vector_time
 ***********************************************************************************/
vector_time::vector_time(std::size_t index, std::size_t count)
    : _time(count), _idx(index)
{
}

bool vector_time::advance()
{
    _time[_idx] += 1;
    return true;
}

std::vector<std::size_t> vector_time::time() const
{
    return _time;
}

void vector_time::merge(const vector_time &other)
{
    for(std::size_t i = 0; i < _time.size(); ++i)
        _time[i] = std::max(_time[i], other._time[i]);
}

bool vector_time::operator==(const vector_time &o) const
{
    return _time == o._time;
}

bool vector_time::operator!=(const vector_time &o) const
{
    return !operator==(o);
}

bool vector_time::operator<=(const vector_time &o) const
{
    return _time <= o._time;
}

bool vector_time::operator<(const vector_time &o) const
{
    bool ret = false;
    for(std::size_t i = 0; i < _time.size(); ++i)
        ret |= _time[i] < o._time[i];
    return operator<=(o) && ret;
}

std::ostream &operator<<(std::ostream &o, const vector_time &msg)
{
    internal::vector_time int_time;
    int_time.set_index(msg._idx);
    auto vec = int_time.mutable_vector();
    vec->Resize(msg._time.size(), 0);
    std::copy(msg._time.cbegin(), msg._time.cend(), vec->begin());
    int_time.SerializeToOstream(&o);
    return o;
}

std::istream &operator>>(std::istream &i, vector_time &msg)
{
    internal::vector_time int_time;
    int_time.ParseFromIstream(&i);
    msg._idx = int_time.index();
    auto vec = int_time.vector();
    msg._time.resize(int_time.vector_size());
    std::copy(vec.cbegin(), vec.cend(), msg._time.begin());
    return i;
}

} // namespace ultra
