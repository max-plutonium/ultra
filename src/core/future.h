#ifndef FUTURE_H
#define FUTURE_H

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include "result.h"

namespace ultra { namespace core {

class state_base
{
    using ptr_type = details::ResultPtr<details::result_base>;
    ptr_type _result;
    std::mutex _lock;
    std::condition_variable _cond;
    using Completer = void (*)(state_base *);
    Completer _completer;
    using HasDeferred = bool (*)(state_base *);
    HasDeferred _has_deferred;
//    std::atomic_size_t _state = 0;

    bool _ready() const noexcept { return static_cast<bool>(_result); }

public:

    state_base(const state_base &) = delete;
    state_base &operator=(const state_base &) = delete;

    details::result_base &wait() {
        _completer(this);
        std::unique_lock<decltype(_lock)> lk(_lock);
        _cond.wait(lk, [&] { return _ready(); });
        return *_result;
    }

    template<typename Rep, typename Period>
    std::future_status
    wait_for(const std::chrono::duration<Rep, Period>& rel)
    {
        std::unique_lock<decltype(_lock)> lk(_lock);
        if(_ready())
            return std::future_status::ready;
        if(_has_deferred(this))
            return std::future_status::deferred;
        if(_cond.wait_for(lk, rel,
            [&] { return _ready(); })) {
            _completer(this);
            return std::future_status::ready;
        }

        return std::future_status::timeout;
    }

    template<typename Clock, typename Duration>
    std::future_status
    wait_until(const std::chrono::time_point<Clock, Duration>& abs)
    {
        std::unique_lock<decltype(_lock)> lk(_lock);
        if (_ready())
            return std::future_status::ready;
        if (_has_deferred(this))
            return std::future_status::deferred;
        if (_cond.wait_until(lk, abs,
            [&] { return _ready(); })) {
            _completer(this);
            return std::future_status::ready;
        }
        return std::future_status::timeout;
    }

protected:
    void _set_result(std::function<ptr_type ()> func) {
        ptr_type res = func();
        std::lock_guard<decltype(_lock)> lk(_lock);
        _result.swap(res);
        _cond.notify_all();
    }
};

} // namespace core

} // namespace ultra

#endif // FUTURE_H
