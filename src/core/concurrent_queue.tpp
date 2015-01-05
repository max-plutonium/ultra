#include "concurrent_queue.h"

namespace ultra { namespace core {

/*! \internal
 *  \brief  Создает узел очереди по переданным аргументам
 *  \param  args    Список параметров, передающийся в конструктор узла.
 *  \return Адрес сконструированного узла.
 */
  template <typename Tp, typename Alloc>
      template<typename... Args>
    auto
    details::basic_forward_queue<Tp, Alloc>::
    _create_node(Args&&... args) -> scoped_node_ptr
    {
        using traits = std::allocator_traits<node_alloc_type>;
        node_alloc_type &alloc = _get_node_allocator();
        scoped_node_ptr node { traits::allocate(alloc, 1), *this };
        traits::construct(alloc, node.get(), std::forward<Args>(args)...);
        return node;
    }

/*! \internal
 *  Встраивает узел по адресу \a p в конец очереди
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

/*! \internal
 *  \brief  Вынимает следующий узел из очереди
 *  \return Адрес отцепленного узла.
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

/*! \internal
 *  \brief  Последовательно выцепляет и уничтожает узлы из списка
 *  \note   Не делает никаких блокировок.
 */
  template <typename Tp, typename Alloc>
    void
    details::basic_forward_queue<Tp, Alloc>::_clear()
    {
        while(_unhook_next());
    }

/*! \internal
 *  \brief  Копирует себе содержимое очереди \a other
 *  \note   Если во время копирования элементов возникнет исключение,
 *  оставляет очередь в первоначальном состоянии.
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

/*! \internal
 *  \brief Добавляет к себе содержимое очереди \a other
 *  перемещением элементов с помощью простого обмена указателей
 */
  template <typename Tp, typename Lock, typename Alloc>
    void
    concurrent_queue<Tp, Lock, Alloc>::
    _append(concurrent_queue<Tp, Lock, Alloc> &&other)
    {
        // to protect deadlocks
        ordered_lock<Lock, Lock> locker { _lock, other._lock };
        if(this->_impl.last)
            this->_impl.last->next = other._impl.next;
        else
            this->_impl.next = other._impl.next;
        this->_impl.last = other._impl.last;
        other._impl.next = nullptr;
        other._impl.last = nullptr;
    }

/*! \internal
 *  \brief Добавляет к себе содержимое очереди \a other копированием элементов
 *  \note  Исходная очередь остается в первоначальном состоянии.
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

/*! Конструктор очереди, инициализирует поля
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc>::concurrent_queue() noexcept
    {
    }

/*! Деструктор очереди
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc>::~concurrent_queue() noexcept
    {
        _lock.lock();
        _base::_clear();
    }

/*!
 *  \brief  Копирует к себе содержимое очереди \a other
 *  \note   При создании узлов может бросать исключение,
 *  в этом случае очередь остается пустой.
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc>::
    concurrent_queue(const concurrent_queue<Tp, Lock, Alloc> &other)
        : concurrent_queue()
    {
        _assign(other);
    }

/*!
 *  \brief  Очищает свое содержимое и копирует к себе
 *  содержимое очереди \a other
 *  \note   При создании узлов может бросать исключение,
 *  в этом случае очередь остается нетронутой.
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

/*! \copydoc concurrent_queue(const concurrent_queue &other)
 */
  template <typename Tp, typename Lock, typename Alloc>
      template <typename Tp2, typename Lock2, typename Alloc2>
    concurrent_queue<Tp, Lock, Alloc>::
    concurrent_queue(concurrent_queue<Tp2, Lock2, Alloc2> const &other)
        : concurrent_queue()
    {
        _assign(other);
    }

/*! \copydoc concurrent_queue &concurrent_queue::operator=(const concurrent_queue &other)
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
 *  \brief Добавляет к себе содержимое очереди \a other перемещением элементов
 */
  template <typename Tp, typename Lock, typename Alloc>
    concurrent_queue<Tp, Lock, Alloc> &
    concurrent_queue<Tp, Lock, Alloc>::
    append(concurrent_queue<Tp, Lock, Alloc> &&other)
    {
        _append(std::move(other)); return *this;
    }

/*!
 *  \brief Добавляет к себе содержимое очереди \a other копированием элементов
 *  \note  Исходная очередь остается в первоначальном состоянии.
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
 *  \brief  Обменивает содержимое очереди с \a other
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
 *  \brief  Обменивает содержимое очереди с \a other
 *  \note   Не делает никаких блокировок.
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
 *  \brief  Конструирует элемент по переданному списку параметров
 *          и добавляет его в очередь
 *  \return Возвращает true, если успешно.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename... Args>
    bool
    concurrent_queue<Tp, Lock, Alloc>::enqueue(Args&&... args)
    {
        static_assert(std::is_constructible<value_type, Args...>::value,
                      "template argument substituting Tp"
            " must be constructible from these arguments");

        typename _base::scoped_node_ptr node
                = _base::_create_node(nullptr, std::forward<Args>(args)...);

        std::lock_guard<Lock> lk(_lock);
        _base::_hook(node.release());
        return true;
    }

/*!
 *  \brief  Берет следующий элемент из очереди и передает
 *          его по ссылке в \a val, если очередь не пуста
 *  \return false, если очередь уже пуста, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
    bool
    concurrent_queue<Tp, Lock, Alloc>::dequeue(value_type &val)
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
 *  \brief  Конструирует элемент по переданному списку параметров
 *          и добавляет его в очередь, не производя никаких блокировок
 *  \return Возвращает true, если успешно.
 */
  template <typename Tp, typename Lock, typename Alloc>
      template<typename... Args>
    bool
    concurrent_queue<Tp, Lock, Alloc>::enqueue_unsafe(Args&&... args)
    {
        static_assert(std::is_constructible<value_type, Args...>::value,
                      "template argument substituting Tp"
            " must be constructible from these arguments");

        typename _base::scoped_node_ptr node
                = _base::_create_node(nullptr, std::forward<Args>(args)...);
        _base::_hook(node.release());
        return true;
    }

/*!
 *  \brief  Берет следующий элемент из очереди, не производя
 *          никаких блокировок, и передает его по ссылке в
 *          \a val, если очередь не пуста
 *  \return false, если очередь уже пуста, иначе true.
 */
  template <typename Tp, typename Lock, typename Alloc>
    bool
    concurrent_queue<Tp, Lock, Alloc>::dequeue_unsafe(value_type &val)
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
