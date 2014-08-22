#ifndef LOCKS_H
#define LOCKS_H

#include <mutex>

namespace ultra { namespace core {

template <typename Tp> class is_lockable
{
  template <typename Up>
    static constexpr bool check(decltype(std::declval<Up>().lock())*,
                                decltype(std::declval<Up>().unlock())*)
    { return true; }

  template <typename>
    static constexpr bool check(...) { return false; }

public:
    enum { value = check<Tp>(nullptr, nullptr) };
};


template <typename Lockable1, typename Lockable2>
class ordered_lock
{
    static_assert(is_lockable<Lockable1>::value
                  && is_lockable<Lockable2>::value,
        "ordered_lock only works with lockable types");
    std::pair<Lockable1*, Lockable2*> locks;
    bool locked;

    ordered_lock(Lockable1 &l1, Lockable2 &l2, bool is_locked) noexcept
        : locks(std::addressof(l1), std::addressof(l2))
        , locked(is_locked) { }

public:
    constexpr ordered_lock() noexcept
        : locks(nullptr, nullptr), locked(false) { }

    ordered_lock(Lockable1 &l1, Lockable2 &l2)
        : ordered_lock(l1, l2, false) { lock(); }

    ordered_lock(Lockable1 &l1, Lockable2 &l2, std::defer_lock_t)
        noexcept : ordered_lock(l1, l2, false) { }

    ordered_lock(Lockable1 &l1, Lockable2 &l2, std::adopt_lock_t)
        noexcept : ordered_lock(l1, l2, true) { }

    ~ordered_lock()
    {
        if(locked)
            unlock();
    }

    ordered_lock(const ordered_lock&) = delete;
    ordered_lock &operator=(const ordered_lock&) = delete;

    ordered_lock(ordered_lock &&other) noexcept
        : ordered_lock() { swap(other); }

    ordered_lock &operator=(ordered_lock &&other) noexcept
    {
        ordered_lock(std::move(other)).swap(*this);
        return *this;
    }

    void lock()
    {
        using namespace std;
        if(!locks.first && !locks.second && !locked) {
            locked = true;
            return;
        }
        else if(!locks.first || !locks.second)
            throw system_error(make_error_code(
                    errc::operation_not_permitted));
        else if(locked)
            throw system_error(make_error_code(
                    errc::resource_deadlock_would_occur));
        else if(static_cast<void*>(locks.first)
           < static_cast<void*>(locks.second))
        {
            locks.first->lock();
            locks.second->lock();
        }
        else
        {
            locks.second->lock();
            locks.first->lock();
        }

        locked = true;
    }

    void unlock()
    {
        using namespace std;
        if(!locks.first && !locks.second && locked) {
            locked = false;
            return;
        }
        else if(!locks.first || !locks.second)
            throw system_error(make_error_code(
                    errc::operation_not_permitted));
        else if(!locked)
            throw system_error(make_error_code(
                    errc::operation_not_permitted));
        else if(static_cast<void*>(locks.first)
           < static_cast<void*>(locks.second))
        {
            locks.first->unlock();
            locks.second->unlock();
        }
        else
        {
            locks.second->unlock();
            locks.first->unlock();
        }

        locked = false;
    }

    std::pair<Lockable1*, Lockable2*> release() noexcept
    {
        std::pair<Lockable1*, Lockable2*> ret;
        std::swap(ret, locks);
        return ret;
    }

    void swap(ordered_lock &other) noexcept
    {
        std::swap(locks, other.locks);
        std::swap(locked, other.locked);
    }

    bool owns_lock() const noexcept
    { return locked; }

    explicit operator bool() const noexcept
    { return owns_lock(); }

}; // class ordered_lock

} // namespace core

} // namespace ultra

#endif // LOCKS_H
