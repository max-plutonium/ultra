#ifndef INTERP_H
#define INTERP_H

#include <random>
#include "device.h"

namespace ultra {

class interp : public device
{
    std::uniform_int_distribution<int> _distr;
    std::mt19937_64 _generator;

public:
    interp(const address &a, node *parent = nullptr, int prio = 0)
        : device(a, parent, prio)
    { }
    ~interp();

    // task interface
public:
    virtual void run() override
    {
        std::string ss;
        _in_ports.at(_distr(_generator) % _in_ports.size()) >> ss;
        if(!ss.empty())
            _out_ports.at(_distr(_generator) % _out_ports.size()) << ss;
    }
};

} // namespace ultra

#endif // INTERP_H
