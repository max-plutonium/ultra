#include "system.h"

#if defined _WIN32 || defined _WIN64 || defined _WINDOWS
#   define __windows__
#endif

#include <cmath>    // ceil
#include <cstring>  // memset
#include <cassert>
#include <algorithm> // std::min, std::max
#include <csetjmp>

#ifdef __unix__
#   include <unistd.h>          // sysconf
#   include <sys/resource.h>    // rlimit
#   include <signal.h>          // MINSIGSTKSZ
#   include <fcntl.h>           // open, close
#   include <sys/mman.h>        // mmap, munmap, mprotect

#elif defined __windows__
#   include <windows.h>

#endif

extern "C" void fini_context();
extern "C" void __attribute__((nothrow, noreturn))
init_context(void *sp, void *ip, void *arg)
{
#ifdef __MINGW32__
    asm volatile(
        "movl %0, %%esp \n"
        "movl %%esp, %%ebp \n"
        "pushl %1 \n"
        "pushl %2 \n"
        "pushl %3 \n"
        "retl \n"
        :: "r"(sp), "r"(arg)
        , "r"(&fini_context), "r"(ip)
        : "%esp");
#else
#   error "platform not supported"
#endif

    __builtin_unreachable();
}

extern "C" void __attribute__((nothrow))
fini_context()
{

}

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

struct machine_context_sjlj : system::machine_context
{
    static thread_local void *_data;
    void __attribute__((nothrow, noreturn)) resume_context();
    bool __attribute__((nothrow)) pause_context();

    machine_context_sjlj(void *sp, void *ip)
        : _sp(sp), _ip(ip) { }

private:
    void *_sp;
    void *_ip;
    jmp_buf _jump_context;
    bool _started = false;
};

/*static*/ thread_local
void *machine_context_sjlj::_data = 0;

void machine_context_sjlj::resume_context()
{
    if(__builtin_expect(_started, true)) {
        longjmp(_jump_context, 1);
    } else {
        _started = true;
        init_context(_sp, _ip, _data);
    }
    __builtin_unreachable();
}

bool machine_context_sjlj::pause_context()
{
    if(__builtin_expect(!_started, false))
        _started = true;
    return 1 == setjmp(_jump_context);
}

/*static*/
system::machine_context_ptr
system::make_context(stack &astack, void (*func)(void *))
{
    return std::make_shared<machine_context_sjlj>(
                astack.second, reinterpret_cast<void *>(func));
}

/*static*/
void *system::switch_context(system::machine_context_ptr &from,
                             const system::machine_context_ptr &to, void *data)
{
    if(!from)
        from = std::make_shared<machine_context_sjlj>(nullptr, nullptr);

    machine_context_sjlj *from_ctx
            = dynamic_cast<machine_context_sjlj*>(from.get());
    machine_context_sjlj *to_ctx
            = dynamic_cast<machine_context_sjlj*>(to.get());
    assert(from_ctx && to_ctx);

    if(!from_ctx->pause_context()) {
        machine_context_sjlj::_data = data;
        to_ctx->resume_context();
        __builtin_unreachable();
    }

    data = machine_context_sjlj::_data;
    machine_context_sjlj::_data = 0;
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
    return MINSIGSTKSZ + 16;
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

    return { result_size, static_cast<char*>(ptr) + result_size };
}

/*static*/
void system::deallocate_stack(stack &astack)
{
    assert(astack.second);
    assert(minimum_stacksize() <= astack.first);
    assert(is_stack_unbound() || (maximum_stacksize() >= astack.first));

    void *ptr = static_cast<char*>(astack.second) - astack.first;

#ifdef __unix__
    // POSIX.4 (POSIX.1b-1993, _POSIX_C_SOURCE=199309L)
    ::munmap(ptr, astack.first);

#elif defined __windows__
    ::VirtualFree(ptr, 0, MEM_RELEASE);

#endif
}

} // namespace core

} // namespace ultra
