#ifndef LOCKS_H
#define LOCKS_H

#include <mutex>

namespace ultra { namespace core {

/*!
 * \internal
 */
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

/*!
 * \brief Класс спин-блокировки
 */
class spinlock
{
    char _pad[8];

    void _sleep() const noexcept;

public:
    using duration_type = std::chrono::microseconds;

    /*!
     * \brief Создает спин-блокировку
     *
     * \param duration_usecs Количество микросекунд ожидания на
     * захваченной блокировке
     */
    explicit spinlock(unsigned int duration_usecs = 0) noexcept;

    /*!
     * Уничтожает спин-блокировку
     */
    ~spinlock() noexcept;

#ifndef DOXYGEN
    spinlock(const spinlock&) = delete;
    spinlock &operator=(const spinlock&) = delete;
    spinlock(spinlock&&) = delete;
    spinlock &operator=(spinlock&&) = delete;
#endif

    /*!
     * \brief Захватывает блокировку
     */
    void lock() noexcept;

    /*!
     * \brief Снимает блокировку
     */
    void unlock() noexcept;

    /*!
     * \brief Пытается захватить блокировку \a n раз
     *
     * \return true, если блокировка захвачена
     */
    bool try_lock(unsigned n = 1) noexcept;

    /*!
     * \brief Пытается захватить блокировку до наступления времени \a atime
     *
     * \return true, если блокировка захвачена
     */
  template <typename Clock, typename Duration>
    inline bool
    try_lock_until(const std::chrono::time_point<Clock, Duration> &atime)
    {
        while(!try_lock()) {
            if(std::chrono::system_clock::now() >= atime)
                return false;
            _sleep();
        }
        return true;
    }

    /*!
     * \brief Пытается захватить блокировку в течение \a rtime
     *
     * \return true, если блокировка захвачена
     */
  template <typename Rep, typename Period>
    inline bool
    try_lock_for(const std::chrono::duration<Rep, Period> &rtime)
    { return try_lock_until(std::chrono::system_clock::now() + rtime); }

    /*!
     * \brief Возвращает время ожидания на захваченной блокировке
     */
    unsigned get_sleep_dur() noexcept;

    /*!
     * \brief Устанавливает время ожидания на захваченной блокировке \a usecs
     */
    void set_sleep_dur(unsigned usecs) noexcept;

    /// Устанавливает период ожидания спинлока \a rtime
  template <typename Rep, typename Period>
    inline void
    set_sleep_dur(const std::chrono::duration<Rep, Period> &rtime)
    { set_sleep_dur(std::chrono::duration_cast<duration_type>(rtime).count()); }

    /// Сбрасывает период ожидания спинлока
    inline void reset_sleep_dur() noexcept { set_sleep_dur(0); }
};


/*!
 * \brief Необходим для упорядоченного захвата блокировок
 *
 * В некоторых алгоритмах требуется захватить сразу две
 * блокировки, что может привести к зависанию, когда два или более
 * потоков ждут друг друга. ordered_lock последовательно
 * захватывает блокировки согласно их адресам, т.е. будучи вызван
 * из разных потоков все равно будет захватывать блокировки
 * в одном и том же порядке.
 *
 * Обычное применение - в конструкторах копирования или перемещения
 * в классах, содержащих блокировки.
 */
template <typename Lockable1, typename Lockable2>
class ordered_lock
{
#ifndef DOXYGEN
    static_assert(is_lockable<Lockable1>::value
                  && is_lockable<Lockable2>::value,
        "ordered_lock only works with lockable types");
#endif

    std::pair<Lockable1 *, Lockable2 *> locks;
    bool locked;

    ordered_lock(Lockable1 &l1, Lockable2 &l2, bool is_locked) noexcept
        : locks(std::addressof(l1), std::addressof(l2))
        , locked(is_locked) { }

public:
    /*!
     * \brief Конструирует пустой объект
     */
    constexpr ordered_lock() noexcept
        : locks(nullptr, nullptr), locked(false) { }

    /*!
     * \brief Конструирует объект и захватывает блокировки
     */
    ordered_lock(Lockable1 &l1, Lockable2 &l2)
        : ordered_lock(l1, l2, false) { lock(); }

    /*!
     * \brief Конструирует объект без захвата блокировок
     *
     * Необходим для того, чтобы захватить блокировки позже.
     *
     * \sa lock()
     */
    ordered_lock(Lockable1 &l1, Lockable2 &l2, std::defer_lock_t)
        noexcept : ordered_lock(l1, l2, false) { }

    /*!
     * \brief Конструирует объект без захвата блокировок
     *
     * Необходим для того, чтобы сконструировать объект из
     * уже захваченных блокировок.
     *
     * \sa unlock()
     */
    ordered_lock(Lockable1 &l1, Lockable2 &l2, std::adopt_lock_t)
        noexcept : ordered_lock(l1, l2, true) { }

    /*!
     * \brief Освобождает блокировки, если они были захвачены
     */
    ~ordered_lock()
    {
        if(locked)
            unlock();
    }

    ordered_lock(const ordered_lock&) = delete;
    ordered_lock &operator=(const ordered_lock&) = delete;

    /*!
     * \brief Конструирует объект из другого объекта перемещением
     */
    ordered_lock(ordered_lock &&other) noexcept
        : ordered_lock() { swap(other); }

    /*!
     * \brief Присваивает себе блокировки из \a other
     */
    ordered_lock &operator=(ordered_lock &&other) noexcept {
        ordered_lock(std::move(other)).swap(*this);
        return *this;
    }

    /*!
     * \brief Захватывает блокировки
     */
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

    /*!
     * \brief Освобождает блокировки
     */
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

    /*!
     * \brief Отдает блокировки без освобождения
     *
     * \return Пару из хранящихся адресов блокировок.
     */
    std::pair<Lockable1 *, Lockable2 *> release() noexcept
    {
        std::pair<Lockable1*, Lockable2*> ret;
        std::swap(ret, locks);
        return ret;
    }

    /*!
     * \brief Обменивает блокировки с \a other
     */
    void swap(ordered_lock &other) noexcept
    {
        std::swap(locks, other.locks);
        std::swap(locked, other.locked);
    }

    /*!
     * \brief Возвращает true, если блокировки захвачены
     */
    bool owns_lock() const noexcept
    { return locked; }

    /*!
     * \copydoc owns_lock()
     */
    explicit operator bool() const noexcept
    { return owns_lock(); }

}; // class ordered_lock

} // namespace core

} // namespace ultra

#endif // LOCKS_H
