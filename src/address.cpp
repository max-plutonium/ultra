#include "address.h"
#include <boost/functional/hash.hpp>

namespace ultra
{
    std::size_t address_hash::operator()(const address &c) const
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, c.x());
        boost::hash_combine(seed, c.y());
        boost::hash_combine(seed, c.z());
        return seed;
    }

} // namespace ultra
