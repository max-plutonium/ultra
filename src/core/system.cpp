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
            /* reserve space for context structure at top
             * of context stack and align stack on 16 byte boundary */ \
            "subq $32, %[stack] \n" \
            "movq %[stack], %[context] \n" \
            "andq $-16, %[stack] \n" \
            \
            /* reserve space for the return address and pointer
             * to context function, save real function to stack and
             * then save stack pointer for context function */ \
            "leaq -16(%[stack]), %[stack] \n" \
            "movq %[function], 0(%[stack]) \n" \
            "movq %[stack], 0(%[context]) \n" \
            \
            /* compute abs address of label init_context
             * and save it as address of context function */ \
            "leaq init_context(%%rip), %[function] \n" \
            "movq %[function], 8(%[context]) \n" \
            \
            /* compute abs address of label fini_context
             * and save it as return address for context function */ \
            "leaq fini_context(%%rip), %[function] \n" \
            "movq %[function], 8(%[stack]) \n" \
            "jmp bail \n" \
            \
            /* start context function */ \
            "init_context: \n" \
            "movq %%rax, %%rdi \n" \
            "retq \n" \
            \
            /* exit application with zero exit code */ \
            "fini_context: \n" \
            "xorq %%rdi, %%rdi \n" \
            "call _exit \n" \
            "hlt \n bail: \n" \
            \
        : [context] "=&rm"(ctx_ptr) : [stack] "r"(sp), [function] "r"(ip) \
        : "memory");

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
            /* save return address and stack pointer */ \
            "movq $resume_context, 8(%[prev]) \n" \
            "movq %%rsp, 0(%[prev]) \n" \
            \
            /* restore stack pointer and jump */ \
            "movq 0(%[next]), %%rsp \n" \
            "jmpq *8(%[next]) \n" \
            \
            /* restore rflags and registers */ \
            "resume_context: popfq \n" \
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
            /* reserve space for context structure at top
             * of context stack and align stack on 16 byte boundary */ \
            "subl $16, %[stack] \n" \
            "movl %[stack], %[context] \n" \
            "andl $-16, %[stack] \n" \
            \
            /* reserve space for the return address, args and
             * pointer to context function, save real function to stack
             * and then save stack pointer for context function */ \
            "leal -24(%[stack]), %[stack] \n" \
            "movl %[function], 0(%[stack]) \n" \
            "movl %[stack], 0(%[context]) \n" \
            \
            /* compute abs address of label init_context
             * and save it as address of context function */ \
            "call 1f \n" \
            "1: popl %[function] \n" \
            "addl $init_context - 1b, %[function] \n" \
            "movl %[function], 4(%[context]) \n" \
            \
            /* compute abs address of label fini_context
             * and save it as return address for context function */ \
            "call 2f \n" \
            "2: popl %[function] \n" \
            "addl $fini_context - 2b, %[function] \n" \
            "movl %[function], 4(%[stack]) \n" \
            "jmp bail \n" \
            \
            /* start context function */ \
            "init_context: \n" \
            "movl %%eax, 8(%%esp) \n" \
            "retl \n" \
            \
            /* exit application with zero exit code */ \
            "fini_context: \n" \
            "xorl %%eax, %%eax \n" \
            "movl %%eax, (%%esp) \n" \
            "call _exit \n" \
            "hlt \n bail: \n" \
            \
        : [context] "=&rm"(ctx_ptr) : [stack] "r"(sp), [function] "r"(ip) \
        : "memory");

#   define switch_context_native(ctx_from, ctx_to, arg) \
        asm volatile( \
            /* save registers and eflags */ \
            "pushl %%ebp \n" \
            "pushl %%ebx \n" \
            "pushl %%edi \n" \
            "pushl %%esi \n" \
            "pushfl \n" \
            \
            /* save return address and stack pointer */ \
            "movl $resume_context, 4(%[prev]) \n" \
            "movl %%esp, 0(%[prev]) \n" \
            \
            /* restore stack pointer and jump */ \
            "movl 0(%[next]), %%esp \n" \
            "jmpl *4(%[next]) \n" \
            \
            /* restore eflags and registers */ \
            "resume_context: popfl \n" \
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
            /* reserve space for context structure at top
             * of context stack and align stack on 16 byte boundary */ \
            "sub %[stack], %[stack], #16 \n" \
            "mov %[context], %[stack] \n" \
            "bic %[stack], %[stack], #15 \n" \
            \
            /* reserve space for the return address and pointer
             * to context function, save real function to stack and
             * then save stack pointer for context function */ \
            "sub %[stack], %[stack], #8 \n" \
            "str %[function], [%[stack], #4] \n" \
            "str %[stack], [%[context], #0] \n" \
            \
            /* compute abs address of label init_context
             * and save it as address of context function */ \
            "adr %[function], init_context \n" \
            "str %[function], [%[context], #4] \n" \
            \
            /* compute abs address of label fini_context
             * and save it as return address for context function */ \
            "adr %[function], fini_context \n" \
            "str %[function], [%[stack], #0] \n" \
            "b bail \n" \
            \
            /* start context function */ \
            "init_context: \n" \
            "ldmia %%sp!, { %%lr, %%pc } \n" \
            \
            /* exit application with zero exit code */ \
            "fini_context: \n" \
            "mov %%r0, #0 \n" \
            "bl _exit \n" \
            "bail: \n" \
            \
        : [context] "=&rm"(ctx_ptr) : [stack] "r"(sp), [function] "r"(ip) \
        : "memory");

#   define switch_context_native(ctx_from, ctx_to, arg) \
        asm volatile( \
            /* save registers */ \
            "stmdb %%sp!, { %%r4-%%r12, %%lr } \n" \
            \
            /* save CPSR flag register */ \
            "mrs %%lr, cpsr \n" \
            "stmdb %%sp!, { %%lr } \n" \
            \
            /* save stack pointer and return address */ \
            "adr %%lr, resume_context \n" \
            "stmia %[prev], { %%sp, %%lr } \n" \
            \
            /* set argument for context function,
             * restore stack pointer and jump */ \
            "mov %%r0, %[data] \n" \
            "ldmia %[next], { %%sp, %%pc } \n" \
            \
            /* restore CPSR flag register*/ \
            "resume_context: \n" \
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
