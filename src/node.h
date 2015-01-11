#ifndef NODE_H
#define NODE_H

#include <deque>
#include <memory>

#include "address.h"
#include "logic_time.h"
#include "ultra.h"

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

class port;

class device : public node, public task
{
protected:
    std::deque<std::shared_ptr<port>> _in_ports, _out_ports;

public:
    explicit device(const address &a, node *parent = nullptr, int prio = 0);
    ~device();

    void add_input_from(port &);
    void add_output_to(const port &);

    // task interface
private:
    virtual void run() = 0;
    friend class field;
};

using device_ptr = std::shared_ptr<device>;

} // namespace ultra

#endif // NODE_H
