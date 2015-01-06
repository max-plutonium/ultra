#include "device.h"

namespace ultra {

device::device(const address &a, node *parent, int prio)
    : node(a, parent), task(prio)
{
}

device::~device()
{
}

void device::add_input_from(port &p)
{
    _in_ports.emplace_back(ultra::openmode::out, p.get_address(), this);
    p.connect(_in_ports.back());
}

void device::add_output_to(const port &p)
{
    _out_ports.emplace_back(ultra::openmode::in, p.get_address(), this);
    _out_ports.back().connect(p);
}

} // namespace ultra
