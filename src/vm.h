#ifndef VM_H
#define VM_H

#include "node.h"
#include <sstream>

namespace ultra {

class port;

class pipe
{
    struct impl;
    impl *const d;

public:
    pipe();
    ~pipe();
    void attach(port *);
};

enum class openmode : int {
    app = std::ios_base::app,
    ate = std::ios_base::ate,
    binary = std::ios_base::binary,
    in = std::ios_base::in,
    out = std::ios_base::out,
    trunc = std::ios_base::trunc,
    inout = std::ios_base::in | std::ios_base::out
};

class port : public node, public std::stringstream
{
public:
    port(address a, ultra::openmode om);
    ~port();

    // node interface
protected:
    virtual void message(const scalar_message_ptr &);
};

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
