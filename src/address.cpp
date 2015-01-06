#include "address.h"
#include <boost/functional/hash.hpp>
#include "msg.pb.h"

namespace ultra {

address::address(int cluster, int field, int node) noexcept
    : _cluster(cluster), _field(field), _node(node)
{ }

address::address(const std::initializer_list<int> &il)
    : _cluster(il.begin()[0]), _field(il.begin()[1]), _node(il.begin()[2])
{ }

bool address::operator==(const address &o) const
{
    return (_cluster == o._cluster && _field == o._field && _node == o._node);
}

bool address::operator!=(const address &o) const
{
    return !operator==(o);
}

std::size_t address_hash::operator()(const address &c) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, c.cluster());
    boost::hash_combine(seed, c.field());
    boost::hash_combine(seed, c.node());
    return seed;
}

std::ostream &operator<<(std::ostream &o, const address &msg)
{
    internal::address int_address;
    int_address.set_cluster(msg.cluster());
    int_address.set_field(msg.field());
    int_address.set_node(msg.node());
    int_address.SerializeToOstream(&o);
    return o;
}

std::istream &operator>>(std::istream &i, address &msg)
{
    internal::address int_address;
    int_address.ParseFromIstream(&i);
    msg._cluster = int_address.cluster();
    msg._field = int_address.field();
    msg._node = int_address.node();
    return i;
}

} // namespace ultra
