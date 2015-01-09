#ifndef FIELD_H
#define FIELD_H

#include <unordered_map>

#include "address.h"
#include "interp.h"
#include "port.h"

namespace ultra {

class field
{
    int _nr_cluster, _nr_space, _nr_field;
    std::unordered_map<address, interp_ptr, address_hash> _interps;
    std::unordered_map<address, port_ptr, address_hash> _ports;

public:
    field(int cluster_number, int space_number, int field_number)
        : _nr_cluster(cluster_number), _nr_space(space_number), _nr_field(field_number)
    { }

    ~field() { }

    interp_ptr create_interp(int number)
    {
        address addr(_nr_cluster, _nr_space, _nr_field, number);
        if(_interps.count(addr))
            return _interps.at(addr);
        auto ret = std::make_shared<interp>(addr);
        _interps.insert({ addr, ret });
        return ret;
    }

    // task interface
public:
    virtual void run()
    {
        for(const std::pair<address, interp_ptr> d : _interps)
            d.second->run();
    }
};

} // namespace ultra

#endif // FIELD_H
