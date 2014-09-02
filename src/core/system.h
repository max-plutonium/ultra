#ifndef SYSTEM_H
#define SYSTEM_H

#include <utility>

namespace ultra { namespace core {

class system
{
public:
    static std::size_t get_pagesize();
    static std::size_t get_pagecount(std::size_t memsize);

    using stack = std::pair<std::size_t, void*>;
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
