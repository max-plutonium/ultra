#include "node.h"

namespace ultra {

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

} // namespace ultra
