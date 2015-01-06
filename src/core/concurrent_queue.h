#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include "locks.h"
#include <memory>
#include <type_traits>

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
            node(node *anext, Args&&... args) : next(anext)
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

        using scoped_node_ptr = std::unique_ptr<node, basic_forward_queue<Tp, Alloc>&>;

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
    static_assert(std::is_copy_constructible<Tp>::value
                  || std::is_move_constructible<Tp>::value,
        "concurrent_queue requires copyable or movable template argument");

    static_assert(is_lockable<Lock>::value,
        "concurrent_queue only works with lockable type");

    using _base = details::basic_forward_queue<Tp, Alloc>;

    mutable Lock _lock;

    // Можно конструировать и присваивать из любых совместимых типов
  template <typename Tp2, typename Lock2, typename Alloc2>
    void _assign(concurrent_queue<Tp2, Lock2, Alloc2> const&);

    void _append(concurrent_queue&&);

    // Можно добавлять очереди с любыми совместимыми типами
  template <typename Tp2, typename Lock2, typename Alloc2>
    void _append(concurrent_queue<Tp2, Lock2, Alloc2> const&);

public:
    using allocator_type = Alloc;
    using value_type = Tp;
    using size_type = std::size_t;

    /// Возвращает аллокатор, используемый очередью
    allocator_type get_allocator() const noexcept
    { return allocator_type(_base::_get_node_allocator()); }

    concurrent_queue() noexcept;
    ~concurrent_queue() noexcept;

    concurrent_queue(concurrent_queue const&);
    concurrent_queue &operator=(concurrent_queue const&);

    // Можно конструировать и присваивать из любых совместимых типов
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue(concurrent_queue<Tp2, Lock2, Alloc2> const&);
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue &operator=(concurrent_queue<Tp2, Lock2, Alloc2> const&);

    concurrent_queue &append(concurrent_queue&&);

  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue &append(concurrent_queue<Tp2, Lock2, Alloc2> const&);

    /// Перемещает содержимое \a other к себе
    concurrent_queue(concurrent_queue &&other) noexcept
        : concurrent_queue() { swap(other); }

    /// Очищает свое содержимое и переносит к себе содержимое
    /// очереди \a other
    concurrent_queue &operator=(concurrent_queue &&other) noexcept
    { concurrent_queue(std::move(other)).swap(*this); return *this; }

  template <typename Lock2>
    concurrent_queue(concurrent_queue<Tp, Lock2, Alloc> &&other) noexcept
        : concurrent_queue() { swap(other); }

  template <typename Lock2>
    concurrent_queue &operator=(concurrent_queue<Tp, Lock2, Alloc> &&other) noexcept
    { concurrent_queue(std::move(other)).swap(*this); return *this; }

    // Нельзя перемещать из других типов
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue(concurrent_queue<Tp2, Lock2, Alloc2>&&) = delete;
  template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue &operator=(concurrent_queue<Tp2, Lock2, Alloc2>&&) = delete;

    /// Возвращает true, если мгновенный размер очереди равен нулю
    inline bool empty() const noexcept
    { std::lock_guard<Lock> lk(_lock); return !_base::_impl.last; }

    inline void clear() noexcept
    { concurrent_queue<Tp, Lock, Alloc>().swap(*this); }

  template <typename Lock2>
    void swap(concurrent_queue<Tp, Lock2, Alloc>&) noexcept;

  template <typename Lock2>
    void swap_unsafe(concurrent_queue<Tp, Lock2, Alloc>&) noexcept;

  template <typename... Args>
    inline bool enqueue(Args&&... args);

    inline bool dequeue(value_type &val);

  template <typename... Args>
    inline bool push(Args&&... args)
    { return enqueue(std::forward<Args>(args)...); }

    inline bool pop(value_type &val)
    { return dequeue(val); }

  template <typename... Args>
    inline bool enqueue_unsafe(Args&&... args);

    inline bool dequeue_unsafe(value_type &val);

  template <typename... Args>
    inline bool push_unsafe(Args&&... args)
    { return enqueue_unsafe(std::forward<Args>(args)...); }

    inline bool pop_unsafe(value_type &val)
    { return dequeue_unsafe(val); }

  template <typename Tpa, typename Locka, typename Alloca>
    friend class concurrent_queue;

}; // class concurrent_queue

} // namespace core

} // namespace ultra

#include "concurrent_queue.tpp"

#endif // CONCURRENT_QUEUE_H
