#include "node.h"
#include "port.h"

namespace ultra {

/************************************************************************************
     node
 ***********************************************************************************/
void node::_add_child(node *child)
{
    _children.push_back(child);
}

node::node(const address &a, ultra::node *parent)
    : _addr(a), _time(0), _parent(parent)
{
}

node::~node()
{
    for(node *n : _children)
        delete n;
}

address node::get_address() const
{
    return _addr;
}

scalar_time node::get_time() const
{
    return _time;
}


/************************************************************************************
     device
 ***********************************************************************************/
device::device(const address &a, node *parent, int prio)
    : node(a, parent), task(prio)
{
}

device::~device()
{
}

void device::add_input_from(port &p)
{
    _in_ports.emplace_back(std::make_shared<port>(ultra::openmode::out, p.get_address(), this));
    p.connect(*_in_ports.back().get());
}

void device::add_output_to(const port &p)
{
    _out_ports.emplace_back(std::make_shared<port>(ultra::openmode::in, p.get_address(), this));
    _out_ports.back()->connect(p);
}

} // namespace ultra
