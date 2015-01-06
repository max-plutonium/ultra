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
    void loop();
    void post_message(port_message);
};

} // namespace ultra

#endif // VM_H
