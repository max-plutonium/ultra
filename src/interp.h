#ifndef INTERP_H
#define INTERP_H

#include "device.h"

namespace ultra {

class interp : public device
{
public:
    interp(const address &a, node *parent = nullptr, int prio = 0)
        : device(a, parent, prio)
    { }
    ~interp();

    // task interface
public:
    virtual void run() override
    {
    }
};

} // namespace ultra

#endif // INTERP_H
