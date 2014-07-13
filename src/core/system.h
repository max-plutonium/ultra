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

    struct machine_context : std::enable_shared_from_this<machine_context>
    {
        virtual ~machine_context() = default;
    };

    using stack = std::pair<std::size_t, void *>;
    using machine_context_ptr = std::shared_ptr<machine_context>;
    static machine_context_ptr make_context(stack &astack, void (*func)(void *));
    static void *switch_context(machine_context_ptr &from,
                                const machine_context_ptr &to, void *data);

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
