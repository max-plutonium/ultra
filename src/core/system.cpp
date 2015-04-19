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
#   include <sys/wait.h>        // waitpid

#elif defined __windows__
#   include <windows.h>

#else
#   error "platform not supported"

#endif

#if defined __x86_64__ /* UNIX System V ABI only */
    /*
     *  Description of the initial information
     *  at the top of context stack:
     *
     *  0   RFLAGS
     *  8   R12
     *  16  R13
     *  24  R14
     *  32  R15
     *  40  RBX
     *  48  RBP
     *  56  startRIP
     *  64  ctxRIP
     *  72  ctxRET
     *  80  free space
     *  96  RSP
     *  104 parent context
     */

asm volatile(
        "__make_context: \n"

        /* reserve space for context structure at top
         * of context stack and align stack on 16 byte boundary */
        "subq $16, %rdi \n"
        "andq $-16, %rdi \n"
        "movq %rdi, %rax \n"

        /* reserve space for context data */
        "leaq -96(%rdi), %rdi \n"

        /* save init frame pointer value pointed to null */
        "leaq 80(%rdi), %rcx \n"
        "movq %rcx, 48(%rdi) \n"

        /* save pointer to real function */
        "movq %rsi, 64(%rdi) \n"

        /* compute abs address of label init_context
         * and save it as start address of context */
        "leaq init_context(%rip), %rsi \n"
        "movq %rsi, 56(%rdi) \n"

        /* compute abs address of label fini_context
         * and save it as return address for context function */
        "leaq fini_context(%rip), %rsi \n"
        "movq %rsi, 72(%rdi) \n"

        /* save stack pointer for context function */
        "movq %rdi, 0(%rax) \n"
        "retq \n"

        /* start trampoline for context function */
        "init_context: \n"
        "movq %rax, %rdi \n"
        "retq \n"

        /* try to install parent context (if we have),
         * otherwise exit application with zero exit code */
        "fini_context: \n"
        "cmpq $0, 16(%rsp) \n"
        "je 1f \n"
        "leaq 16(%rsp), %rdi \n"
        "movq 24(%rsp), %rsi \n"
        "movq %rax, %rdx \n"
        "call switch_context@plt \n" /* exit application if returns */
        "1: xorq %rdi, %rdi \n"
        "call _exit@plt \n"
        "hlt \n");

asm volatile(
        "__switch_context: \n"

        /* save registers and rflags */
        "pushq %rbp \n"
        "pushq %rbx \n"
        "pushq %r15 \n"
        "pushq %r14 \n"
        "pushq %r13 \n"
        "pushq %r12 \n"
        "pushfq \n"

        /* exchange stack pointer */
        "movq %rsp, 0(%rdi) \n"
        "movq 0(%rsi), %rsp \n"

        /* restore rflags and registers */
        "popfq \n"
        "popq %r12 \n"
        "popq %r13 \n"
        "popq %r14 \n"
        "popq %r15 \n"
        "popq %rbx \n"
        "popq %rbp \n"

        "movq %rdx, %rax \n"
        "retq \n");

#elif defined __MINGW32__ /* MINGW System V ABI */
    /*
     *  Description of the initial information
     *  at the top of context stack:
     *
     *  0   EFLAGS
     *  4   ESI
     *  8   EDI
     *  12  EBX
     *  16  EBP
     *  20  startEIP
     *  24  ctxEIP
     *  28  ctxRET
     *  32  space for args (16 bytes)
     *  48  ESP
     *  52  parent context
     */

asm volatile(
        "___make_context: \n"
        "movl 4(%esp), %edx \n"
        "movl 8(%esp), %ecx \n"

        /* reserve space for context structure at top
         * of context stack and align stack on 16 byte boundary */
        "subl $8, %edx \n"
        "andl $-16, %edx \n"
        "movl %edx, %eax \n"

        /* reserve space for context data */
        "leal -48(%edx), %edx \n"

        /* save init frame pointer value pointed to null */
        "leal 36(%edx), %ebx \n"
        "movl %ebx, 16(%edx) \n"

        /* save pointer to real function */
        "movl %ecx, 24(%edx) \n"

        /* compute abs address of label init_context
         * and save it as start address of context */
        "call 1f \n"
        "1: popl %ecx \n"
        "addl $init_context - 1b, %ecx \n"
        "movl %ecx, 20(%edx) \n"

        /* compute abs address of label fini_context
         * and save it as return address for context function */
        "call 2f \n"
        "2: popl %ecx \n"
        "addl $fini_context - 2b, %ecx \n"
        "movl %ecx, 28(%edx) \n"

        /* save stack pointer for context function */
        "movl %edx, 0(%eax) \n"
        "retl \n"

        /* start trampoline for context function */
        "init_context: \n"
        "movl %eax, 8(%esp) \n"
        "retl \n"

        /* try to install parent context (if we have),
         * otherwise exit application with zero exit code */
        "fini_context: \n"
        "cmpl $0, 20(%esp) \n"
        "je 3f \n"
        "pushl %eax \n"
        "pushl 24(%esp) \n"
        "leal 24(%esp), %ecx \n"
        "pushl %ecx \n"
        "call _switch_context \n" /* exit application if returns */
        "3: xorl %eax, %eax \n"
        "movl %eax, (%esp) \n"
        "call _exit \n"
        "hlt \n");

asm volatile(
        "___switch_context: \n"
        "movl 4(%esp), %edx \n"
        "movl 8(%esp), %ecx \n"
        "movl 12(%esp), %eax \n"

        /* save registers and eflags */
        "pushl %ebp \n"
        "pushl %ebx \n"
        "pushl %edi \n"
        "pushl %esi \n"
        "pushfl \n"

        /* exchange stack pointer */
        "movl %esp, 0(%edx) \n"
        "movl 0(%ecx), %esp \n"

        /* restore eflags and registers */
        "popfl \n"
        "popl %esi \n"
        "popl %edi \n"
        "popl %ebx \n"
        "popl %ebp \n"
        "retl \n");

#elif defined __arm__ /* AAPCS ABI */
    /*
     *  Description of the initial information
     *  at the top of context stack:
     *
     *  0   CPSR
     *  4   R4
     *  8   R5
     *  12  R6
     *  16  R7
     *  20  R8
     *  24  R9
     *  28  R10
     *  32  R11 (FP)
     *  36  R12 (IP)
     *  40  R14 (LR)
     *  44  ctxLR
     *  48  ctxPC
     *  52  free space
     *  68  SP
     *  72  parent context
     */

asm volatile(
        "__make_context: \n"
        "push { %lr } \n"

        /* reserve space for context structure at top
         * of context stack and align stack on 16 byte boundary */
        "sub %r0, %r0, #8 \n"
        "bic %r0, %r0, #15 \n"
        "mov %r3, %r0 \n"

        /* reserve space for context data */
        "sub %r0, %r0, #68 \n"

        /* save init frame pointer value pointed to null */
        "add %r2, %r0, #52 \n"
        "str %r2, [%r0, #32] \n"

        /* save pointer to real function */
        "str %r1, [%r0, #48] \n"

        /* compute abs address of label init_context
         * and save it as start address of context */
        "adr %r1, init_context \n"
        "str %r1, [%r0, #40] \n"

        /* compute abs address of label fini_context
         * and save it as return address for context function */
        "adr %r1, fini_context \n"
        "str %r1, [%r0, #44] \n"

        /* save stack pointer for context function */
        "str %r0, [%r3, #0] \n"
        "mov %r0, %r3 \n"
        "pop { %pc } \n"

        /* start trampoline for context function */
        "init_context: \n"
        "pop { %lr } \n"
        "pop { %pc } \n"

        /* try to install parent context (if we have),
         * otherwise exit application with zero exit code */
        "fini_context: \n"
        "ldr %r1, [%sp, #20] \n"
        "cmp %r1, #0 \n"
        "beq 1f \n"
        "mov %r2, %r0 \n"
        "add %r0, %sp, #16 \n"
        "bl switch_context \n" /* exit application if returns */
        "1: mov %r0, #0 \n"
        "bl _exit \n");

asm volatile(
        "__switch_context: \n"

        /* save registers */
        "stmdb %sp!, { %r4-%r12, %lr } \n"

        /* save CPSR flag register */
        "mrs %lr, cpsr \n"
        "stmdb %sp!, { %lr } \n"

        /* exchange stack pointer */
        "str %sp, [%r0, #0] \n"
        "ldr %sp, [%r1, #0] \n"

        /* restore CPSR flag register*/
        "ldmia %sp!, { %lr } \n"
        "msr cpsr, %lr \n"

        /* restore registers */
        "ldmia %sp!, { %r4-%r12, %lr } \n"

        "mov %r0, %r2 \n"
        "bx %lr \n");

#else
#   error "platform not supported"

#endif

#include "ultra_global.h"


namespace ultra { namespace core {

extern "C" ULTRA_INTERNAL machine_context *
__make_context(void *sp, context_entry ip);

extern "C" ULTRA_INTERNAL std::intptr_t
__switch_context(machine_context *, machine_context *, std::intptr_t);

static thread_local machine_context *t_current_context;

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
machine_context *
system::make_context(const machine_stack &astack,
    context_entry entry, machine_context *parent)
{
    machine_context *ret = __make_context(astack.second, entry);
    if(parent)
        (*ret)[1] = parent;
    return ret;
}

extern "C" ULTRA_INTERNAL std::intptr_t
switch_context(machine_context *from, machine_context *to, std::intptr_t data)
{
    t_current_context = to;
    return __switch_context(from, to, data);
}

/*static*/
bool system::inside_context()
{
    return static_cast<bool>(t_current_context);
}

/*static*/
machine_context *system::current_context()
{
    return t_current_context;
}

/*static*/ std::intptr_t
system::install_context(machine_context *ctx, std::intptr_t data)
{
    if(!ctx)
        return -1;

    machine_context this_ctx, *old_parent
            = (t_current_context) ? t_current_context : &this_ctx;
    machine_context **parent = reinterpret_cast<machine_context **>(&(*ctx)[1]);
    std::swap(*parent, old_parent);
    data = switch_context(*parent, ctx, data);
    std::swap(*parent, old_parent);
    if(&this_ctx == t_current_context)
        t_current_context = 0;
    return data;
}

/*static*/
std::intptr_t system::yield_context(std::intptr_t data)
{
    machine_context *const parent = t_current_context
        ? static_cast<machine_context *>((*t_current_context)[1])
        : nullptr;
    if(!parent) return -1;
    return switch_context(t_current_context, parent, data);
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
machine_stack system::allocate_stack(std::size_t stack_size)
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
void system::deallocate_stack(machine_stack &astack)
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

/*static*/
int system::fork_process(int (*entry)(int, char **, char **), int argc, char **argv, char **envp)
{
    int res = -1;

#ifdef __unix__
    const pid_t pid = ::fork();
    res = (pid > 0) ? pid : entry(argc, argv, envp);
#endif
    return res;
}

/*static*/
int system::join_process(int pid)
{
#ifdef __unix__
    int exit_status = 0;
    /*int res = */::waitpid(pid, &exit_status, WUNTRACED);
    return exit_status;
#endif
}

/*static*/
void system::kill_process(int pid)
{
#ifdef __unix__
    ::kill(pid, SIGKILL);
#endif
}

} // namespace core

} // namespace ultra
