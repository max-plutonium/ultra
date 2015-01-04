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
    void register_port(port *);
    void unregister_port(port *);
    void loop();
    void post_message(scalar_message_ptr);
};

} // namespace ultra

#endif // VM_H
