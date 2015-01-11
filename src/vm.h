#ifndef VM_H
#define VM_H

#include <map>

#include "field.h"
#include "port.h"

namespace ultra {

class vm
{
    struct impl;
    impl *d = nullptr;

    std::map<int, field> _fields;

public:
    vm(int argc, const char **argv);
    ~vm();

    static vm *instance();
    void wait_for_done();
    void loop();
    void post_message(port_message);

    friend struct timed_task;
    friend struct scheduler;
};

} // namespace ultra

#endif // VM_H
