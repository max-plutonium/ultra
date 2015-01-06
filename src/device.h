#ifndef DEVICE_H
#define DEVICE_H

#include "port.h"
#include "task.h"

namespace ultra {

class device : public node, public task
{
protected:
    std::deque<port> _in_ports, _out_ports;

public:
    explicit device(const address &a, node *parent = nullptr, int prio = 0);
    ~device();

    void add_input_from(port &);
    void add_output_to(const port &);

    // task interface
private:
    virtual void run() = 0;
};

} // namespace ultra

#endif // DEVICE_H
