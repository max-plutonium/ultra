#ifndef SYSTEM_H
#define SYSTEM_H

#include <utility>
#include <memory>

namespace ultra { namespace core {

class system
{
public:
    static std::size_t get_pagesize();
    static std::size_t get_pagecount(std::size_t memsize);

    using machine_context = void *[2];
    using stack = std::pair<std::size_t, void *>;

    static machine_context *
    make_context(const stack &astack, void (*func)(std::intptr_t));

    static std::intptr_t
    switch_context(machine_context &from,
        const machine_context &to, std::intptr_t data = 0);

    static bool is_stack_unbound();
    static std::size_t default_stacksize();
    static std::size_t minimum_stacksize();
    static std::size_t maximum_stacksize();
    static stack allocate_stack(std::size_t stack_size);
    static void deallocate_stack(stack &);
};

} // namespace core

} // namespace ultra

#endif // SYSTEM_H
