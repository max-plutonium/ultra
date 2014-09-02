#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include <cstdint>

namespace ultra { namespace core {

struct stack
{
    std::size_t size = 0;
    void *sp = 0;
};

class stack_allocator
{
public:
    static bool is_stack_unbound();
    static std::size_t default_stacksize();
    static std::size_t minimum_stacksize();
    static std::size_t maximum_stacksize();
    void allocate(stack&, std::size_t size);
    void deallocate(stack&);
};

} // namespace core

} // namespace ultra

#endif // STACK_ALLOCATOR_H
