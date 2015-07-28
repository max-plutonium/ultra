#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <memory>
#include <type_traits>
#include <condition_variable>

#include "locks.h"

namespace ultra { namespace core {

namespace details {

    /*!
     * \internal
     */
  template <typename Tp, typename Alloc>
    struct basic_forward_queue
    {
        struct node
        {
            node *next;
            Tp   t;

          template <typename... Args>
            node(node *anext, Args &&...args) : next(anext)
              , t(std::forward<Args>(args)...) { }
        };

        // Ребинд к типу аллокатора узла
        typedef typename Alloc::template rebind<node>::other
            node_alloc_type;

        // Структура корня списка. Используем EBO,
        // наследуясь от аллокатора узла
        struct queue_impl : public node_alloc_type
        {
            node  *next = nullptr, *last = nullptr;

            queue_impl() : node_alloc_type() { }
            queue_impl(const node_alloc_type &a) : node_alloc_type(a) { }
            queue_impl(node_alloc_type &&a) : node_alloc_type(std::move(a)) { }

            queue_impl(const queue_impl&) = default;
            queue_impl &operator=(const queue_impl&) = default;

            queue_impl(queue_impl &&other) noexcept
                : node_alloc_type(std::move(other)) { swap(other); }
            queue_impl &operator=(queue_impl &&other) noexcept
            { queue_impl(std::move(other)).swap(*this); return *this; }

            inline void swap(queue_impl &other) noexcept {
                std::swap(next, other.next);
                std::swap(last, other.last);
            }
        };

        void operator()(node *ptr)
        {
            using traits = std::allocator_traits<node_alloc_type>;
            node_alloc_type &alloc = _get_node_allocator();
            traits::destroy(alloc, ptr);
            traits::deallocate(alloc, ptr, 1);
        }

        using scoped_node_ptr = std::unique_ptr<node, basic_forward_queue<Tp, Alloc> &>;

        queue_impl _impl;

        // Возвращает ссылку на аллокатор узла
        inline node_alloc_type &_get_node_allocator() noexcept
        { return *static_cast<node_alloc_type*>(&_impl); }

        // Возвращает константную ссылку на аллокатор узла
        inline const node_alloc_type &_get_node_allocator() const noexcept
        { return *static_cast<const node_alloc_type*>(&_impl); }

      template <typename... Args>
        scoped_node_ptr _create_node(Args&&...);

        inline void  _hook(node*) noexcept;
        inline scoped_node_ptr _unhook_next() noexcept;
        void _clear();
        inline bool _empty() const noexcept { return !_impl.last; }

    }; // struct basic_forward_queue

} // namespace details

/*!
 * \brief Потокобезопасная блокирующая очередь
 *
 * Представляет собой простой односвязный список, операции над которым
 * сериализованы с помощью блокировки.
 *
 * \tparam Tp Класс хранимых объектов.
 * \tparam Lock Класс блокировки.
 * \tparam Alloc Аллокатор, который будет использован для выделения памяти.
 */
template <typename Tp, typename Lock, typename Alloc = std::allocator<Tp>>
class concurrent_queue : protected details::basic_forward_queue<Tp, Alloc>
{
#ifndef DOXYGEN
    static_assert(std::is_copy_constructible<Tp>::value
                  || std::is_move_constructible<Tp>::value,
        "concurrent_queue requires copyable or movable template argument");

    static_assert(is_lockable<Lock>::value,
        "concurrent_queue only works with lockable type");
#endif

    using _base = details::basic_forward_queue<Tp, Alloc>;
    using _cond_type = typename std::conditional<
        std::is_same<Lock, std::mutex>::value,
        std::condition_variable, std::condition_variable_any>::type;

    mutable Lock _lock;
    _cond_type _cond;
    bool _closed = false;

    // Можно конструировать и присваивать из любых совместимых типов
  template <typename Tp2, typename Lock2, typename Alloc2>
    void _assign(concurrent_queue<Tp2, Lock2, Alloc2> const&);

  template <typename Lock2>
    void _append(concurrent_queue<Tp, Lock2, Alloc> &&);

    // Можно добавлять очереди с любыми совместимыми типами
  template <typename Tp2, typename Lock2, typename Alloc2>
    void _append(concurrent_queue<Tp2, Lock2, Alloc2> const&);

    bool _wait(std::unique_lock<Lock> &lk);

  template<typename Clock, typename Duration>
    bool _wait(std::unique_lock<Lock> &lk,
        const std::chrono::time_point<Clock, Duration> &atime);

  template <typename Rep, typename Period>
    bool _wait(std::unique_lock<Lock> &lk,
        const std::chrono::duration<Rep, Period> &rtime);

public:
    using allocator_type = Alloc;
    using value_type = Tp;
    using size_type = std::size_t;

    /// Возвращает аллокатор, используемый очередью
    allocator_type get_allocator() const noexcept
    { return allocator_type(_base::_get_node_allocator()); }

    concurrent_queue() noexcept;
    ~concurrent_queue();

    concurrent_queue(concurrent_queue const&);
    concurrent_queue &operator=(concurrent_queue const&);

    // Можно конструировать и присваивать из любых совместимых типов

    /// \copydoc concurrent_queue(const concurrent_queue &other)
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue(concurrent_queue<Tp2, Lock2, Alloc2> const&);

    /// \copydoc concurrent_queue::operator=(const concurrent_queue &other)
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue &operator=(concurrent_queue<Tp2, Lock2, Alloc2> const&);

  template <typename Lock2>
    concurrent_queue &append(concurrent_queue<Tp, Lock2, Alloc> &&other);

  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue &append(concurrent_queue<Tp2, Lock2, Alloc2> const&);

    /// Перемещает содержимое \a other к себе
    concurrent_queue(concurrent_queue &&other) noexcept
        : concurrent_queue() { swap(other); }

    /// \copydoc concurrent_queue(concurrent_queue &&other)
  template <typename Lock2>
    concurrent_queue(concurrent_queue<Tp, Lock2, Alloc> &&other) noexcept
        : concurrent_queue() { swap(other); }

    /// Очищает свое содержимое и переносит к себе содержимое
    /// очереди \a other
    concurrent_queue &operator=(concurrent_queue &&other) noexcept
    { concurrent_queue(std::move(other)).swap(*this); return *this; }

    /// \copydoc operator=(concurrent_queue &&other)
  template <typename Lock2>
    concurrent_queue &operator=(concurrent_queue<Tp, Lock2, Alloc> &&other) noexcept
    { concurrent_queue(std::move(other)).swap(*this); return *this; }

#ifndef DOXYGEN
    // Нельзя перемещать из других типов
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue(concurrent_queue<Tp2, Lock2, Alloc2> &&) = delete;
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue &operator=(concurrent_queue<Tp2, Lock2, Alloc2> &&) = delete;
#endif

    /// Возвращает true, если мгновенный размер очереди равен нулю
    inline bool empty() const
    { std::lock_guard<Lock> lk(_lock); return _base::_empty(); }

    /// Возвращает true, если очередь закрыта
    inline bool closed() const
    { std::lock_guard<Lock> lk(_lock); return _closed; }

    /// Очищает содержимое очереди
    inline void clear() noexcept
    { concurrent_queue<Tp, Lock, Alloc>().swap(*this); }

    void close();

    /// Возвращает ссылку на объект внутренней блокировки очереди
    inline Lock &underlying_lock() const noexcept { return _lock; }

  template <typename Lock2>
    void swap(concurrent_queue<Tp, Lock2, Alloc> &) noexcept;

  template <typename Lock2>
    void swap_unsafe(concurrent_queue<Tp, Lock2, Alloc> &) noexcept;

  template <typename... Args>
    bool push(Args &&...args);

    bool pull(value_type &val);

    bool wait_pull(value_type &val);

  template <typename Clock, typename Duration>
    bool wait_pull(const std::chrono::time_point<Clock, Duration> &atime,
                   value_type &val);

  template <typename Rep, typename Period>
    bool wait_pull(const std::chrono::duration<Rep, Period> &rtime,
                   value_type &val);

  template <typename... Args>
    bool push_unsafe(Args &&...args);

    bool pull_unsafe(value_type &val);

  template <typename Tpa, typename Locka, typename Alloca>
    friend class concurrent_queue;

}; // class concurrent_queue

} // namespace core

} // namespace ultra

#include "concurrent_queue.ipp"

#endif // CONCURRENT_QUEUE_H
