#include <atomic>
#include <thread>
#include <pthread.h>
#include <sys/time.h>

#include "locks.h"

//#define ULTRA_SPINLOCK_USE_POSIX
#define ULTRA_SPINLOCK_DELAY_REVS 1000

namespace ultra { namespace core {

/************************************************************************************
    spinlock
 ***********************************************************************************/

struct spinlock_impl
{
#ifdef ULTRA_SPINLOCK_USE_POSIX
    pthread_spinlock_t _device;
#else
    std::atomic_flag   _flag = ATOMIC_FLAG_INIT;
#endif
    std::atomic_uint   _sleep_dur;

    spinlock_impl(unsigned int duration_usecs) noexcept
        : _sleep_dur(duration_usecs)
    {
    #ifdef ULTRA_SPINLOCK_USE_POSIX
        pthread_spin_init(&_device, PTHREAD_PROCESS_PRIVATE);
    #endif
    }

    ~spinlock_impl()
    {
    #ifdef ULTRA_SPINLOCK_USE_POSIX
        pthread_spin_destroy(&_device);
    #endif
    }

    static inline spinlock_impl *s_get(spinlock *s) noexcept
    { return reinterpret_cast<spinlock_impl*>(s); }

    static inline const spinlock_impl *s_get(const spinlock *s) noexcept
    { return reinterpret_cast<const spinlock_impl*>(s); }

};  //  struct spinlock_impl


void spinlock::_sleep() const noexcept
{
    auto t = spinlock_impl::s_get(this)
             ->_sleep_dur.load(std::memory_order_acquire);
    if(t)
    {
        try {
            std::this_thread::sleep_for(duration_type(t));
        } catch(...) { }
    }
    else
    {
        std::uint_fast32_t i = ULTRA_SPINLOCK_DELAY_REVS;
        while(i--) { }
    }
}

spinlock::spinlock(unsigned int duration_usecs /*= 0*/) noexcept
{
    new (_pad) spinlock_impl(duration_usecs);
}

spinlock::~spinlock() noexcept
{
    reinterpret_cast<spinlock_impl*>(_pad)->~spinlock_impl();
}

void spinlock::lock() noexcept
{
#ifdef ULTRA_SPINLOCK_USE_POSIX
    while(pthread_spin_trylock(&spinlock_impl::s_get(this)->_device))
#else
    while(spinlock_impl::s_get(this)
        ->_flag.test_and_set(std::memory_order_acquire))
#endif
        _sleep();
}

void spinlock::unlock() noexcept
{
#ifdef ULTRA_SPINLOCK_USE_POSIX
    pthread_spin_unlock(&spinlock_impl::s_get(this)->_device);
#else
    spinlock_impl::s_get(this)->
        _flag.clear(std::memory_order_release);
#endif
}

bool spinlock::try_lock(unsigned n /*= 1*/) noexcept
{
#ifdef ULTRA_SPINLOCK_USE_POSIX
    while(pthread_spin_trylock(&spinlock_impl::s_get(this)->_device)) {
#else
    while(spinlock_impl::s_get(this)
        ->_flag.test_and_set(std::memory_order_acquire)) {
#endif
        if(!--n) return false;
        _sleep();
    }
    return true;
}

unsigned spinlock::get_sleep_dur() noexcept
{
    return spinlock_impl::s_get(this)->_sleep_dur.load(std::memory_order_relaxed);
}

void spinlock::set_sleep_dur(unsigned usecs) noexcept
{
    spinlock_impl::s_get(this)->_sleep_dur.store(usecs, std::memory_order_release);
}

} // namespace core

} // namespace ultra
