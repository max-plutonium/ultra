#include "address.h"
#include <boost/functional/hash.hpp>

namespace ultra
{

address::address(int ax, int ay, int az) noexcept
    : _x(ax), _y(ay), _z(az)
{ }

address::address(const std::initializer_list<int> &il)
    : _x(il.begin()[0]), _y(il.begin()[1]), _z(il.begin()[2])
{ }

bool address::operator==(const address &o) const
{
    return (_x == o._x && _y == o._y && _z == o._z);
}

bool address::operator!=(const address &o) const
{
    return !operator==(o);
}

std::size_t address_hash::operator()(const address &c) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, c.x());
    boost::hash_combine(seed, c.y());
    boost::hash_combine(seed, c.z());
    return seed;
}


} // namespace ultra
