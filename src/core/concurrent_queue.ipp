#include "concurrent_queue.h"

namespace ultra { namespace core {

/*!
 * \internal
 *
 * \brief Создает узел очереди по переданным аргументам
 *
 * \param args Список параметров, передающийся в конструктор узла.
 * \return Адрес сконструированного узла.
 */
  template <typename Tp, typename Alloc>
      template<typename... Args>
    auto
    details::basic_forward_queue<Tp, Alloc>::
    _create_node(Args &&...args) -> scoped_node_ptr
    {
        using traits = std::allocator_traits<node_alloc_type>;
        node_alloc_type &alloc = _get_node_allocator();
        auto guarded_ptr = std::__allocate_guarded(alloc);
        traits::construct(alloc, guarded_ptr.get(), std::forward<Args>(args)...);
        scoped_node_ptr node { guarded_ptr.get(), *this };
        guarded_ptr = nullptr;
        return node;
    }

/*!
 * \internal
 *
 * Встраивает узел по адресу \a p в конец очереди
 */
  template <typename Tp, typename Alloc>
    void
    details::basic_forward_queue<Tp, Alloc>::
    _hook(node *p) noexcept
    {
        if(_impl.last)
            _impl.last->next = p;
        else
            _impl.next = p;
        _impl.last = p;
    }

/*!
 * \internal
 *
 * \brief Вынимает следующий узел из очереди
 *
 * \return Адрес отцепленного узла.
 */
  template <typename Tp, typename Alloc>
    auto
    details::basic_forward_queue<Tp, Alloc>::
    _unhook_next() noexcept -> scoped_node_ptr
    {
        scoped_node_ptr node { _impl.next, *this };
        if(!node)
            return node;
        else if(!_impl.next->next)
            _impl.last = nullptr;
        _impl.next = _impl.next->next;
        return node;
    }

/*!
 * \internal
 *
 * \brief Последовательно выцепляет и уничтожает узлы из списка
 *
 * \note Не делает никаких блокировок.
 */
  template <typename Tp, typename Alloc>
    void
    details::basic_forward_queue<Tp, Alloc>::_clear()
    {
        while(_unhook_next());
    }

/*!
 * \internal
 *
 * \brief Копирует себе содержимое очереди \a other
 *
 * \note Если во время копирования элементов возникнет исключение,
 * оставляет очередь в первоначальном состоянии.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename Tp2, typename Lock2, typename Alloc2>
    void
    concurrent_queue<Tp, Lock, Alloc>::
    _assign(concurrent_queue<Tp2, Lock2, Alloc2> const &other)
    {
        static_assert(std::is_constructible<Tp, Tp2>::value,
                "template argument substituting Tp in second"
           " queue object declaration must be constructible from"
                     " Tp in first queue object declaration");

        // Деструктор temp должен выполняться после разблокировок
        concurrent_queue<Tp, Lock, Alloc> temp;

        // to protect deadlocks
        ordered_lock<Lock, Lock2> locker { _lock, other._lock };

        // auto нужно, когда присваивается
        // другой тип очереди с другим аллокатором
        for(auto *other_node_ptr = other._impl.next;
            other_node_ptr;
            other_node_ptr = other_node_ptr->next)
        {
            typename _base::scoped_node_ptr
                node_ptr = temp._create_node(
                    nullptr, std::ref(other_node_ptr->t));
            temp._hook(node_ptr.release());
        }

        // other можно отпустить
        std::unique_lock<Lock> locker2 { _lock, std::adopt_lock };
        locker.release();
        other._lock.unlock();

        // Обмениваем внутренности с временным обьектом
        swap_unsafe(temp);
    }

/*!
 * \internal
 *
 * \brief Добавляет к себе содержимое очереди \a other
 * перемещением элементов с помощью простого обмена указателей
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Lock2>
    void
    concurrent_queue<Tp, Lock, Alloc>::
    _append(concurrent_queue<Tp, Lock2, Alloc> &&other)
    {
        // to protect deadlocks
        ordered_lock<Lock, Lock2> locker { _lock, other._lock };
        if(this->_impl.last)
            this->_impl.last->next = other._impl.next;
        else
            this->_impl.next = other._impl.next;
        this->_impl.last = other._impl.last;
        other._impl.next = nullptr;
        other._impl.last = nullptr;
    }

/*!
 * \internal
 *
 * \brief Добавляет к себе содержимое очереди \a other копированием элементов
 *
 * \note Исходная очередь остается в первоначальном состоянии.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename Tp2, typename Lock2, typename Alloc2>
    void
    concurrent_queue<Tp, Lock, Alloc>::
    _append(concurrent_queue<Tp2, Lock2, Alloc2> const &other)
    {
        static_assert(std::is_constructible<Tp, Tp2>::value,
                "template argument substituting Tp in second"
           " queue object declaration must be constructible from"
                     " Tp in first queue object declaration");

        // Деструктор temp должен выполняться после разблокировок
        concurrent_queue<Tp, Lock, Alloc> temp;

        // to protect deadlocks
        ordered_lock<Lock, Lock2> locker { _lock, other._lock };

        // auto нужно, когда присваивается
        // другой тип очереди с другим аллокатором
        for(auto *other_node_ptr = other._impl.next;
            other_node_ptr;
            other_node_ptr = other_node_ptr->next)
        {
            typename _base::scoped_node_ptr
                node_ptr = temp._create_node(
                    nullptr, std::ref(other_node_ptr->t));
            temp._hook(node_ptr.release());
        }

        locker.unlock();

        // Обмениваем внутренности с временным обьектом
        _append(std::move(temp));
    }

/*!
 * \internal
 *
 * \brief Ждет появления элементов в очереди
 *
 * \return true, если очередь была закрыта.
 */
  template <typename Tp, typename Lock, typename Alloc>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    _wait(std::unique_lock<Lock> &lk)
    {
        _cond.wait(lk, [this]() { return !_base::_empty() || _closed; });
        return _closed;
    }

/*!
 * \internal
 *
 * \brief Ждет появления элементов в очереди до наступления времени \a atime
 *
 * \return true, если очередь не пуста или была закрыта.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename Clock, typename Duration>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    _wait(std::unique_lock<Lock> &lk,
          const std::chrono::time_point<Clock, Duration> &atime)
    {
        return _cond.wait_until(lk, atime,
            [this]() { return !_base::_empty() || _closed; });
    }

/*!
 * \internal
 *
 * \brief Ждет появления элементов в очереди в течение \a rtime
 *
 * \return true, если очередь не пуста или была закрыта.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Rep, typename Period>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    _wait(std::unique_lock<Lock> &lk,
          const std::chrono::duration<Rep, Period> &rtime)
    {
        return _cond.wait_for(lk, rtime,
            [this]() { return !_base::_empty() || _closed; });
    }

/*!
 * \brief Конструктор очереди, инициализирует поля
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc>::
    concurrent_queue() noexcept
    {
    }

/*!
 * \brief Деструктор очереди
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc>::
    ~concurrent_queue()
    {
        close();
        _lock.lock();
        _base::_clear();
    }

/*!
 * \brief Копирует к себе содержимое очереди \a other
 *
 * \note При создании узлов может бросать исключение,
 * в этом случае очередь остается пустой.
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc>::
    concurrent_queue(const concurrent_queue<Tp, Lock, Alloc> &other)
        : concurrent_queue()
    {
        _assign(other);
    }

/*!
 * \brief Очищает свое содержимое и копирует к себе
 * содержимое очереди \a other
 *
 * \note При создании узлов может бросать исключение,
 * в этом случае очередь остается нетронутой.
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc> &
    concurrent_queue<Tp, Lock, Alloc>::
    operator=(concurrent_queue<Tp, Lock, Alloc> const &other)
    {
        if(this != std::addressof(other))
            _assign(other);
        return *this;
    }

/*!
 * \copydoc concurrent_queue(const concurrent_queue &other)
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue<Tp, Lock, Alloc>::
    concurrent_queue(concurrent_queue<Tp2, Lock2, Alloc2> const &other)
        : concurrent_queue()
    {
        _assign(other);
    }

/*!
 * \copydoc concurrent_queue::operator=(const concurrent_queue &other)
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue<Tp, Lock, Alloc> &
    concurrent_queue<Tp, Lock, Alloc>::
    operator=(concurrent_queue<Tp2, Lock2, Alloc2> const &other)
    {
        _assign(other); return *this;
    }

/*!
 * \brief Добавляет к себе содержимое очереди \a other перемещением элементов
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Lock2>
    concurrent_queue<Tp, Lock, Alloc> &
    concurrent_queue<Tp, Lock, Alloc>::
    append(concurrent_queue<Tp, Lock2, Alloc> &&other)
    {
        _append(std::move(other)); return *this;
    }

/*!
 * \brief Добавляет к себе содержимое очереди \a other копированием элементов
 *
 * \note Исходная очередь остается в первоначальном состоянии.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue<Tp, Lock, Alloc> &
    concurrent_queue<Tp, Lock, Alloc>::
    append(concurrent_queue<Tp2, Lock2, Alloc2> const &other)
    {
        _append(other); return *this;
    }

/*!
 * \brief Закрывает очередь
 */
  template <typename Tp, typename Lock, typename Alloc>
    void
    concurrent_queue<Tp, Lock, Alloc>::close()
    {
        std::lock_guard<Lock> lk(_lock);
        _closed = true;
        _cond.notify_all();
    }

/*!
 * \brief Обменивает содержимое очереди с \a other
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Lock2>
    void
    concurrent_queue<Tp, Lock, Alloc>::
    swap(concurrent_queue<Tp, Lock2, Alloc> &other) noexcept
    {
        // to protect deadlocks
        ordered_lock<Lock, Lock2> locker(_lock, other._lock);
        swap_unsafe(other);
    }

/*!
 * \brief Обменивает содержимое очереди с \a other
 *
 * \note Не делает никаких блокировок.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Lock2>
    void
    concurrent_queue<Tp, Lock, Alloc>::
    swap_unsafe(concurrent_queue<Tp, Lock2, Alloc> &other) noexcept
    {
        _base::_impl.swap(other._base::_impl);
    }

/*!
 * \brief Конструирует элемент по переданному списку параметров
 * и добавляет его в очередь, если она не закрыта
 *
 * \snippet core/concurrent_queue.cpp push
 *
 * \return Возвращает true, если очередь не закрыта.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename... Args>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    push(Args &&...args)
    {
        static_assert(std::is_constructible<value_type, Args...>::value,
                      "template argument substituting Tp"
            " must be constructible from these arguments");

        typename _base::scoped_node_ptr node
            = _base::_create_node(nullptr, std::forward<Args>(args)...);

        std::lock_guard<Lock> lk(_lock);
        if(_closed) return false;
        if(_base::_empty()) _cond.notify_one();
        _base::_hook(node.release());
        return true;
    }

/*!
 * \brief Берет следующий элемент из очереди и передает его по ссылке
 * в \a val, если очередь не пуста
 *
 * \snippet core/concurrent_queue.cpp pull
 *
 * \return false, если очередь уже пуста, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    pull(value_type &val)
    {
        typename _base::scoped_node_ptr node { nullptr, *this };

        {
            std::lock_guard<Lock> lk(_lock);
            node = _base::_unhook_next();
        }

        if(node) {
            val = std::move_if_noexcept(node->t);
            return true;
        }

        return false;
    }

/*!
 * \brief Ждёт, пока очередь перестанет быть пустой, берет первый
 * элемент из очереди и передает его по ссылке в \a val, если
 * очередь не закрыта
 *
 * \snippet core/concurrent_queue.cpp wait_pull
 *
 * \return false, если очередь уже закрыта, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    wait_pull(value_type &val)
    {
        typename _base::scoped_node_ptr node { nullptr, *this };

        {
            std::unique_lock<Lock> lk(_lock);
            if(_wait(lk)) return false; // если очередь закрыта
            node = _base::_unhook_next();
        }

        if(node) {
            val = std::move_if_noexcept(node->t);
            return true;
        }

        return false;
    }

/*!
 * \brief Ждёт до наступления времени \a atime, пока очередь
 * перестанет быть пустой, берет первый элемент из очереди и передает
 * его по ссылке в \a val, если очередь не закрыта
 *
 * \snippet core/concurrent_queue.cpp wait_pull
 *
 * \return false, если очередь уже закрыта, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Clock, typename Duration>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    wait_pull(const std::chrono::time_point<Clock, Duration> &atime,
              value_type &val)
    {
        typename _base::scoped_node_ptr node { nullptr, *this };

        {
            std::unique_lock<Lock> lk(_lock);
            if(_wait(lk, atime) && _closed) return false; // если очередь закрыта
            node = _base::_unhook_next();
        }

        if(node) {
            val = std::move_if_noexcept(node->t);
            return true;
        }

        return false;
    }

/*!
 * \brief Ждёт в течение \a rtime, пока очередь перестанет быть пустой,
 * берет первый элемент из очереди и передает его по ссылке в \a val, если
 * очередь не закрыта
 *
 * \snippet core/concurrent_queue.cpp wait_pull
 *
 * \return false, если очередь уже закрыта, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Rep, typename Period>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    wait_pull(const std::chrono::duration<Rep, Period> &rtime,
              value_type &val)
    {
        typename _base::scoped_node_ptr node { nullptr, *this };

        {
            std::unique_lock<Lock> lk(_lock);
            if(_wait(lk, rtime) && _closed) return false; // если очередь закрыта
            node = _base::_unhook_next();
        }

        if(node) {
            val = std::move_if_noexcept(node->t);
            return true;
        }

        return false;
    }

/*!
 * \brief Конструирует элемент по переданному списку параметров и,
 * не производя никаких блокировок, добавляет его в очередь, если она не закрыта
 *
 * \return Возвращает true, если очередь не закрыта.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename... Args>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    push_unsafe(Args &&...args)
    {
        static_assert(std::is_constructible<value_type, Args...>::value,
                      "template argument substituting Tp"
            " must be constructible from these arguments");

        if(_closed) return false;
        const bool need_notify = _base::_empty();
        typename _base::scoped_node_ptr node
            = _base::_create_node(nullptr, std::forward<Args>(args)...);
        _base::_hook(node.release());
        if(need_notify) _cond.notify_one();
        return true;
    }

/*!
 * \brief Берет следующий элемент из очереди, не производя никаких
 * блокировок, и передает его по ссылке в \a val, если очередь не пуста
 *
 * \return false, если очередь уже пуста, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
    bool
    concurrent_queue<Tp, Lock, Alloc>::
    pull_unsafe(value_type &val)
    {
        typename _base::scoped_node_ptr node = _base::_unhook_next();

        if(node) {
            val = std::move_if_noexcept(node->t);
            return true;
        }

        return false;
    }

} // namespace core

} // namespace ultra
