#ifndef RESULT_H
#define RESULT_H

#include <exception>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace ultra { namespace core {

struct result_base
{
protected:
    std::exception_ptr _exception;
    result_base(const result_base&) = delete;
    result_base &operator=(const result_base&) = delete;
    using destroyer = void (*)(result_base *);
    destroyer _destroyer;

public:
    struct deleter {
        void operator()(result_base *r) const;
    };

protected:
    result_base(destroyer d);
};

template <typename Tp> class result : public result_base
{
private:
    static void destroy(result_base *thiz) {
        delete static_cast<result *>(thiz);
    }

    std::aligned_storage<sizeof(Tp), std::alignment_of<Tp>::value> _storage;
    Tp *_ptr = nullptr;

public:
    result() : result_base(&destroy) { }
    ~result() { if(_ptr) _ptr->~Tp(); }

    void set(const Tp &res) {
        _ptr = reinterpret_cast<Tp *>(&_storage);
        ::new(_ptr) Tp(res);
    }

    void set(Tp &&res) {
        _ptr = reinterpret_cast<Tp *>(&_storage);
        ::new(_ptr) Tp(std::move(res));
    }

    Tp &get() noexcept { return *_ptr; }

    bool is_ready() const { return static_cast<bool>(_ptr); }

    using result_type = Tp;

protected:
    result(destroyer d) : result_base(d) { }
};

template <typename Tp> class result<Tp &> : public result_base
{
private:
    static void destroy(result_base *thiz) {
        delete static_cast<result *>(thiz);
    }

    Tp *_ptr = nullptr;

public:
    result() : result_base(&destroy) { }

    void set(Tp &res) noexcept { _ptr = std::addressof(res); }

    Tp &get() noexcept { return *_ptr; }

    bool is_ready() const { return static_cast<bool>(_ptr); }

    using result_type = Tp &;

protected:
    result(destroyer d) : result_base(d) { }
};

template <> struct result<void> : public result_base
{
private:
    static void destroy(result_base *thiz) {
        delete static_cast<result *>(thiz);
    }

public:
    result() : result_base(&destroy) { }

    void set(void *) noexcept { }

    void *get() noexcept { return nullptr; }

    bool is_ready() const { return true; }

    using ResultType = void;

protected:
    result(destroyer d) : result_base(d) { }
};

template <typename Tp, typename Alloc>
struct result_alloc final : public result<Tp>, public Alloc
{
    using allocator_type = typename std::allocator_traits<Alloc>::
        template rebind_alloc<result_alloc>;

private:
    static void destroy(result_base *thiz) {
        using traits = std::allocator_traits<allocator_type>;
        allocator_type a;
        traits::destroy(a, static_cast<result_alloc *>(thiz));
        traits::deallocate(a, static_cast<result_alloc *>(thiz), 1);
    }

public:
    result_alloc(const Alloc &a) : result<Tp>(&destroy), Alloc(a) { }
};

template<typename Result>
  using ResultPtr = std::unique_ptr<Result, result_base::deleter>;

template<typename Tp, typename Allocator>
static ResultPtr<result_alloc<Tp, Allocator>>
allocate_result(const Allocator &a)
{
    using result_type = result_alloc<Tp, Allocator>;
    using traits = std::allocator_traits<typename result_type::allocator_type>;
    typename traits::allocator_type a2(a);
    result_type *ptr = traits::allocate(a2, 1);

    try {
        traits::construct(a2, ptr, a);

    } catch(...) {
        traits::deallocate(a2, ptr, 1);
        throw;
    }
    return ResultPtr<result_type>(ptr);
}

template<typename Tp, typename Up>
static ResultPtr<result<Tp>>
allocate_result(const std::allocator<Up> &a)
{
    return ResultPtr<result<Tp>>(new result<Tp>);
}

} // namespace core

} // namespace ultra

#endif // RESULT_H
