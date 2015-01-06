#ifndef FIELD_H
#define FIELD_H

#include <unordered_map>
#include "address.h"
#include "device.h"
#include "port.h"

namespace ultra {

class field
{
    int _field_number;
    std::unordered_map<address, device_ptr, address_hash> _devices;
    std::unordered_map<address, port_ptr, address_hash> _ports;

public:
    field(int field_number)
        : _field_number(field_number)
    { }

    ~field() { }

    // task interface
public:
    virtual void run()
    {
        for(const std::pair<address, device_ptr> d : _devices)
            d.second->run();
    }
};

} // namespace ultra

#endif // FIELD_H
