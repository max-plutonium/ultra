#ifndef SYSTEM_H
#define SYSTEM_H

#include <utility>
#include <memory>

namespace ultra { namespace core {

using context_entry = std::intptr_t (*)(std::intptr_t);
using machine_context = void *[2];
using machine_stack = std::pair<std::size_t, void *>;

class system
{
public:
    static std::size_t get_pagesize();
    static std::size_t get_pagecount(std::size_t memsize);

    static machine_context *
    make_context(const machine_stack &astack,
        context_entry entry, machine_context *parent = nullptr);

    static bool inside_context();
    static machine_context *current_context();

    static std::intptr_t install_context(
        machine_context *ctx, std::intptr_t data);

    static std::intptr_t yield_context(std::intptr_t data);

    static bool is_stack_unbound();
    static std::size_t default_stacksize();
    static std::size_t minimum_stacksize();
    static std::size_t maximum_stacksize();
    static machine_stack allocate_stack(std::size_t stack_size);
    static void deallocate_stack(machine_stack &);

    static int fork_process(int (*entry)(int, char **, char **), int argc, char **argv, char **envp);
    static int join_process(int pid);
    static void kill_process(int pid);
};

} // namespace core

} // namespace ultra

#endif // SYSTEM_H
