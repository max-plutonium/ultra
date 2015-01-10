#ifndef VM_H
#define VM_H

#include "port.h"

namespace ultra {

class vm
{
    struct impl;
    impl *d = nullptr;

public:
    vm(int argc, const char **argv);
    ~vm();

    static vm *instance();
    void post_message(port_message);

    friend struct timed_task;
    friend class network_task;
    friend struct scheduler;
};

} // namespace ultra

#endif // VM_H
