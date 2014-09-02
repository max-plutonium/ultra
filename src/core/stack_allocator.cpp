#include "stack_allocator.h"

#if defined _WIN32 || defined _WIN64 || defined _WINDOWS
#   define __windows__
#endif

#include <cmath>    // ceil
#include <cstring>  // memset
#include <cassert>
#include <algorithm> // std::min, std::max

#ifdef __unix__
#   include <unistd.h>          // sysconf
#   include <sys/resource.h>    // rlimit
#   include <signal.h>          // MINSIGSTKSZ
#   include <fcntl.h>           // open, close
#   include <sys/mman.h>        // mmap, munmap, mprotect

#elif defined __windows__
#   include <windows.h>

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

static std::size_t get_pagesize()
{
#ifdef __unix__
    // POSIX.1-2001
    return static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));

#elif defined __windows__
    return static_cast<std::size_t>(get_system_info().dwPageSize);

#endif
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

static std::size_t page_count(std::size_t memsize)
{
    return static_cast<std::size_t>(std::ceil(memsize / get_pagesize()));
}


/************************************************************************************
    stack_allocator
 ***********************************************************************************/

bool stack_allocator::is_stack_unbound()
{
#ifdef __unix__
    return RLIM_INFINITY == stacksize_getrlimit().rlim_max;

#elif defined __windows__
    return true;

#endif
}

std::size_t stack_allocator::default_stacksize()
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

std::size_t stack_allocator::minimum_stacksize()
{
    return MINSIGSTKSZ + 16;
}

std::size_t stack_allocator::maximum_stacksize()
{
#ifdef __unix__
    return static_cast<std::size_t>(stacksize_getrlimit().rlim_max);

#elif defined __windows__
    return 1 * 1024 * 1024 * 1024; // 1GB

#endif
}

void stack_allocator::allocate(stack &astack, std::size_t stack_size)
{
    assert(minimum_stacksize() <= stack_size);
    assert(is_stack_unbound() || (maximum_stacksize() >= stack_size));

    // На одну страницу больше - для защищенной страницы
    const std::size_t pages = page_count(stack_size) + 1;
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
    DWORD old_options;
    const BOOL result = ::VirtualProtect(ptr, get_pagesize(),
                            PAGE_READWRITE | PAGE_GUARD /*PAGE_NOACCESS*/, &old_options);
    assert(result != FALSE);

#endif

    astack.size = result_size;
    astack.sp = static_cast<char*>(ptr) + astack.size;
}

void stack_allocator::deallocate(stack &astack)
{
    assert(astack.sp);
    assert(minimum_stacksize() <= astack.size);
    assert(is_stack_unbound() || (maximum_stacksize() >= astack.size));

    void *ptr = static_cast<char*>(astack.sp) - astack.size;

#ifdef __unix__
    // POSIX.4 (POSIX.1b-1993, _POSIX_C_SOURCE=199309L)
    ::munmap(ptr, astack.size);

#elif defined __windows__
    ::VirtualFree(ptr, 0, MEM_RELEASE);

#endif
}


} // namespace core

} // namespace ultra
