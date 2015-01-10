#include "interp.h"
#include "port.h"

namespace ultra {

interp::interp(const address &a, node *parent, int prio)
    : device(a, parent, prio)
{
}

interp::~interp()
{
}

void interp::run()
{
    std::string ss;
    *_in_ports.at(_distr(_generator) % _in_ports.size()) >> ss;
    if(!ss.empty())
        *_out_ports.at(_distr(_generator) % _out_ports.size()) << ss;
}


} // namespace ultra
