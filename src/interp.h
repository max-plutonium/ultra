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
    interp(const address &a, node *parent = nullptr, int prio = 0);
    ~interp();

    // task interface
public:
    virtual void run() override;
};

using interp_ptr = std::shared_ptr<interp>;

} // namespace ultra

#endif // INTERP_H
