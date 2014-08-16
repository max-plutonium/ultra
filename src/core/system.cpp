#include "system.h"

#if defined _WIN32 || defined _WIN64 || defined _WINDOWS
#   define __windows__
#endif

#include <cmath>    // ceil
#include <cstring>  // memset
#include <cassert>
#include <algorithm> // std::min, std::max

#if defined __unix__
#   include <unistd.h>          // sysconf
#   include <sys/resource.h>    // rlimit
#   include <signal.h>          // MINSIGSTKSZ
#   include <fcntl.h>           // open, close
#   include <sys/mman.h>        // mmap, munmap, mprotect

#elif defined __windows__
#   include <windows.h>

#else
#   error "platform not supported"

#endif

#if defined __x86_64__ /* UNIX System V ABI only */
#   define make_context_native(ctx_ptr, sp, ip) \
        asm volatile( \
            "subq $0x20, %[stack] \n" /* reserve space */ \
            "movq $0x0, 0x8(%[stack]) \n" /* return address for context function */ \
            "movq %[function], 0x10(%[stack]) \n" /* save address of context function */ \
            "movq %[stack], 0x18(%[stack]) \n" /* save stack address */ \
            \
            /* compute abs address of context trampoline
             * and save it as future rip */ \
            "leaq context_trampoline(%%rip), %%rax \n" \
            "movq %%rax, (%[stack]) \n" \
            "jmp 1f \n" \
            \
            "context_trampoline: \n" \
            "movq %%rax, %%rdi \n" \
            "jmpq *0x0(%%rcx) \n" \
            \
            /* compute and save address of context structure */ \
            "1: addq $0x10, %[stack] \n" \
            \
        : "=r"(ctx_ptr) : [stack] "0"(sp), [function] "r"(ip) \
        : "memory", "%rax");

#   define switch_context_native(ctx_from, ctx_to, arg) \
        asm volatile( \
            /* save registers and rflags */ \
            "pushq %%rbp \n" \
            "pushq %%rbx \n" \
            "pushq %%r15 \n" \
            "pushq %%r14 \n" \
            "pushq %%r13 \n" \
            "pushq %%r12 \n" \
            "pushfq \n" \
            \
            /* save return address */ \
            "leaq resume_context(%%rip), %%rbx \n" \
            "pushq %%rbx \n" \
            \
            /* exchange rsp */ \
            "movq %%rsp, 0x8(%[prev]) \n" \
            "movq 0x8(%[next]), %%rsp \n" \
            \
            /* jump to saved rip */ \
            "retq \n" \
            "resume_context: \n" \
            \
            /* restore rflags and registers */ \
            "popfq \n" \
            "popq %%r12 \n" \
            "popq %%r13 \n" \
            "popq %%r14 \n" \
            "popq %%r15 \n" \
            "popq %%rbx \n" \
            "popq %%rbp \n" \
            \
        : "+a"(arg) : [prev] "d"(ctx_from), [next] "c"(ctx_to) \
        : "memory");

#elif defined __MINGW32__
#   define make_context_native(ctx_ptr, sp, ip) \
        asm volatile( \
            "subl $0x14, %[stack] \n" /* reserve space */ \
            "movl $0x0, 0x4(%[stack]) \n" /* return address for context function */ \
            "movl %[function], 0xc(%[stack]) \n" /* save address of context function */ \
            "movl %[stack], 0x10(%[stack]) \n" /* save stack address */ \
            \
            /* compute abs address of context trampoline
             * and save it as future eip */ \
            "call 1f \n" \
            "1: popl %%eax \n" \
            "addl $context_trampoline - 1b, %%eax \n" \
            "movl %%eax, (%[stack]) \n" \
            "jmp 2f \n" \
            \
            "context_trampoline: \n" \
            "movl %%eax, 0x4(%%esp) \n" \
            "jmpl *0x0(%%ecx) \n" \
            \
            /* compute and save address of context structure */ \
            "2: addl $0xc, %[stack] \n" \
            \
        : "=r"(ctx_ptr) : [stack] "0"(sp), [function] "r"(ip) \
        : "memory", "%eax");

#   define switch_context_native(ctx_from, ctx_to, arg) \
        asm volatile( \
            /* save registers and eflags */ \
            "pushl %%ebp \n" \
            "pushl %%ebx \n" \
            "pushl %%edi \n" \
            "pushl %%esi \n" \
            "pushfl \n" \
            \
            /* save return address */ \
            "movl $resume_context, %%ebx \n" \
            "pushl %%ebx \n" \
            \
            /* exchange esp */ \
            "movl %%esp, 0x4(%[prev]) \n" \
            "movl 0x4(%[next]), %%esp \n" \
            \
            /* jump to saved eip */ \
            "retl \n" \
            "resume_context: \n" \
            \
            /* restore eflags and registers */ \
            "popfl \n" \
            "popl %%esi \n" \
            "popl %%edi \n" \
            "popl %%ebx \n" \
            "popl %%ebp \n" \
            \
        : "+a"(arg) : [prev] "d"(ctx_from), [next] "c"(ctx_to) \
        : "memory");

#elif defined __arm__
#   define make_context_native(ctx_ptr, sp, ip) \
        asm volatile( \
            "sub %[stack], %[stack], #16 \n" /* reserve space */ \
            "str %[stack], [%[stack], #8] \n" /* save stack address */ \
            "str %[function], [%[stack], #12] \n" /* save address of context function */ \
            \
            /* compute and save address of context structure */ \
            "add %[stack], %[stack], #8 \n" \
            \
        : "=r"(ctx_ptr) : [stack] "0"(sp), [function] "r"(ip) \
        : "memory");

#   define switch_context_native(ctx_from, ctx_to, arg) \
        asm volatile( \
            /* save registers */ \
            "stmdb %%sp!, { %%r4-%%r12, %%lr } \n" \
            \
            /* save cpsr flag register */ \
            "mrs %%lr, cpsr \n" \
            "stmdb %%sp!, { %%lr } \n" \
            \
            /* save stack pointer and return address */ \
            "adr %%lr, resume_context \n" \
            "stmia %[prev], { %%sp, %%lr } \n" \
            \
            /* set ret address and argument for context function */ \
            "mov %%lr, #0 \n"  \
            "mov %%r0, %[data] \n" \
            \
            /* load stack pointer and jump */ \
            "ldmia %[next], { %%sp, %%pc } \n" \
            "resume_context: \n" \
            \
            /* restore cpsr flag register*/ \
            "ldmia %%sp!, { %%lr } \n" \
            "msr cpsr, %%lr \n" \
            \
            /* restore registers */ \
            "ldmia %%sp!, { %%r4-%%r12, %%lr } \n" \
            \
        : [data] "+r"(arg) : [prev] "r"(ctx_from), [next] "r"(ctx_to) \
        : "memory", "r0");

#else
#   error "platform not supported"

#endif

namespace ultra { namespace core {

#ifdef __windows__
static SYSTEM_INFO get_system_info_helper()
{
    SYSTEM_INFO si;
    ::GetSystemInfo(&si);
    return si;
}

static SYSTEM_INFO get_system_info()
{
    static SYSTEM_INFO si = get_system_info_helper();
    return si;
}

#endif // __windows__

/*static*/
std::size_t system::get_pagesize()
{
#ifdef __unix__
    // POSIX.1-2001
    return static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));

#elif defined __windows__
    return static_cast<std::size_t>(get_system_info().dwPageSize);

#endif
}

/*static*/
std::size_t system::get_pagecount(std::size_t memsize)
{
    return static_cast<std::size_t>(std::ceil(memsize / get_pagesize()));
}

/*static*/
system::machine_context *
system::make_context(const stack &astack, void (*func)(std::intptr_t))
{
    machine_context *ret;
    make_context_native(ret, astack.second, func);
    return ret;
}

/*static*/
std::intptr_t system::switch_context(machine_context &from,
                        const machine_context &to, std::intptr_t data)
{
    switch_context_native(&from, &to, data);
    return data;
}

#ifdef __unix__
static rlimit stacksize_getrlimit_helper()
{
    rlimit limit;
    // POSIX.1-2001
    const int result = ::getrlimit(RLIMIT_STACK, &limit);
    assert(result != -1);
    return limit;
}

static rlimit stacksize_getrlimit()
{
    static rlimit limit = stacksize_getrlimit_helper();
    return limit;
}

#endif // __unix__

/*static*/
bool system::is_stack_unbound()
{
#ifdef __unix__
    return RLIM_INFINITY == stacksize_getrlimit().rlim_max;

#elif defined __windows__
    return true;

#endif
}

/*static*/
std::size_t system::default_stacksize()
{
#ifdef __unix__
    std::size_t size = 8 * minimum_stacksize();
    if(is_stack_unbound())
        return size;

    assert(maximum_stacksize() >= minimum_stacksize());
    return maximum_stacksize() == size
            ? size : std::min(size, maximum_stacksize());

#elif defined __windows__
    std::size_t size = 64 * 1024; // 64 kB
    if (is_stack_unbound())
        return (std::max)(size, minimum_stacksize());

    assert(maximum_stacksize() >= minimum_stacksize());
    return maximum_stacksize() == minimum_stacksize()
            ? minimum_stacksize()
            : (std::min)(size, maximum_stacksize());

#endif
}

#ifndef MINSIGSTKSZ
#   define MINSIGSTKSZ 2048
#endif

/*static*/
std::size_t system::minimum_stacksize()
{
    return MINSIGSTKSZ;
}

/*static*/
std::size_t system::maximum_stacksize()
{
#ifdef __unix__
    return static_cast<std::size_t>(stacksize_getrlimit().rlim_max);

#elif defined __windows__
    return 1 * 1024 * 1024 * 1024; // 1GB

#endif
}

/*static*/
system::stack system::allocate_stack(std::size_t stack_size)
{
    assert(minimum_stacksize() <= stack_size);
    assert(is_stack_unbound() || (maximum_stacksize() >= stack_size));

    // На одну страницу больше - для защищенной страницы
    const std::size_t pages = get_pagecount(stack_size) + 1;
    const std::size_t result_size = pages * get_pagesize();
    assert(stack_size > 0 && result_size > 0);

#ifdef __unix__
    const int fd = ::open("/dev/zero", O_RDONLY);
    assert(fd != -1);

    // POSIX.4 (POSIX.1b-1993, _POSIX_C_SOURCE=199309L)
    void *ptr = ::mmap(0, result_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    ::close(fd);

#elif defined __windows__
    void *ptr = ::VirtualAlloc(0, result_size, MEM_COMMIT, PAGE_READWRITE);

#endif

    assert(ptr);

#ifdef __unix__
    // POSIX.1-2001
    // Первая (для стека - последняя) страница - защищенная,
    // при попытке доступа к ней получим SIGSEGV
    const int result = ::mprotect(ptr, get_pagesize(), PROT_NONE);
    assert(result != -1);

#elif defined __windows__
    DWORD old_protection;
    const BOOL result = ::VirtualProtect(ptr, get_pagesize(),
                            PAGE_READWRITE | PAGE_GUARD /*PAGE_NOACCESS*/, &old_protection);
    assert(result != FALSE);

#endif

    return { result_size, static_cast<char *>(ptr) + result_size };
}

/*static*/
void system::deallocate_stack(stack &astack)
{
    assert(astack.second);
    assert(minimum_stacksize() <= astack.first);
    assert(is_stack_unbound() || (maximum_stacksize() >= astack.first));

    void *ptr = static_cast<char *>(astack.second) - astack.first;

#ifdef __unix__
    // POSIX.4 (POSIX.1b-1993, _POSIX_C_SOURCE=199309L)
    ::munmap(ptr, astack.first);

#elif defined __windows__
    ::VirtualFree(ptr, 0, MEM_RELEASE);

#endif
}

} // namespace core

} // namespace ultra
