#ifndef DEVICE_H
#define DEVICE_H

#include "node.h"
#include "ultra.h"

namespace ultra {

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

#endif // DEVICE_H
