#ifndef NODE_H
#define NODE_H

#include "address.h"
#include "logic_time.h"
#include <deque>
#include <memory>

namespace ultra {

class node
{
protected:
    address _addr;
    scalar_time _time;
    std::deque<node *> _children;
    node *_parent;

    void _add_child(node *child);

public:
    explicit node(const address &a = address(), node *parent = nullptr);
    virtual ~node();
    address get_address() const;
    scalar_time get_time() const;
};

using node_ptr = std::shared_ptr<node>;

} // namespace ultra

#endif // NODE_H
