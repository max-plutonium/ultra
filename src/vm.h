#ifndef VM_H
#define VM_H

#include "node.h"

namespace ultra {

class vm
{
    struct impl;
    impl *d = nullptr;

public:
    vm(int argc, const char **argv);
    ~vm();

    static vm *instance();
    void register_node(node_ptr);
    void loop();
    void post_message(scalar_message_ptr);
};

} // namespace ultra

#endif // VM_H
