#include "logic_time.h"
#include "msg.pb.h"

namespace ultra {

/************************************************************************************
    scalar_time
 ***********************************************************************************/
/*!
 * \brief Конструирует скалярное время из числа \a t
 */
scalar_time::scalar_time(std::size_t t)
    : _time(t)
{
}

void scalar_time::advance()
{
    ++_time;
}

/*!
 * \brief Возвращает значение скалярного времени
 */
std::size_t scalar_time::time() const
{
    return _time;
}

/*!
 * \brief Объединяет счетчик скалярного времени с \a other
 */
void scalar_time::merge(const scalar_time &other)
{
    _time = std::max(_time, other._time);
}

/*!
 * \brief Возвращает true, если скалярное время совпадает с \a o
 */
bool scalar_time::operator==(const scalar_time &o) const
{
    return _time == o._time;
}

/*!
 * \brief Возвращает false, если скалярное время совпадает с \a o
 */
bool scalar_time::operator!=(const scalar_time &o) const
{
    return !operator==(o);
}

/*!
 * \brief Возвращает true, если скалярное время меньше либо равно \a o
 */
bool scalar_time::operator<=(const scalar_time &o) const
{
    return _time <= o._time;
}

/*!
 * \brief Возвращает true, если скалярное время меньше чем у \a o
 */
bool scalar_time::operator<(const scalar_time &o) const
{
    return _time < o._time;
}

/*!
 * \brief Сериализует скалярное время в поток
 */
std::ostream &operator<<(std::ostream &o, const ultra::scalar_time &msg)
{
    internal::scalar_time int_time;
    int_time.set_counter(msg._time);
    int_time.SerializeToOstream(&o);
    return o;
}

/*!
 * \brief Десериализует скалярное время из потока
 */
std::istream &operator>>(std::istream &i, ultra::scalar_time &msg)
{
    internal::scalar_time int_time;
    int_time.ParseFromIstream(&i);
    msg._time = int_time.counter();
    return i;
}


/************************************************************************************
    vector_time
 ***********************************************************************************/
/*!
 * \brief Конструирует векторное время
 *
 * \param index Текущий индекс в векторе.
 * \param count Размер вектора.
 */
vector_time::vector_time(std::size_t index, std::size_t count)
    : _time(count), _idx(index)
{
}

void vector_time::advance()
{
    _time[_idx] += 1;
}

/*!
 * \brief Возвращает значение векторного времени
 */
std::vector<std::size_t> vector_time::time() const
{
    return _time;
}

/*!
 * \brief Объединяет счетчик векторного времени с \a other
 */
void vector_time::merge(const vector_time &other)
{
    for(std::size_t i = 0; i < _time.size(); ++i)
        _time[i] = std::max(_time[i], other._time[i]);
}

/*!
 * \brief Возвращает true, если векторное время совпадает с \a o
 */
bool vector_time::operator==(const vector_time &o) const
{
    return _time == o._time;
}

/*!
 * \brief Возвращает false, если векторное время совпадает с \a o
 */
bool vector_time::operator!=(const vector_time &o) const
{
    return !operator==(o);
}

/*!
 * \brief Возвращает true, если векторное время меньше либо равно \a o
 */
bool vector_time::operator<=(const vector_time &o) const
{
    return _time <= o._time;
}

/*!
 * \brief Возвращает true, если векторное время меньше чем у \a o
 */
bool vector_time::operator<(const vector_time &o) const
{
    bool ret = false;
    for(std::size_t i = 0; i < _time.size(); ++i)
        ret |= _time[i] < o._time[i];
    return operator<=(o) && ret;
}

/*!
 * \brief Сериализует векторное время в поток
 */
std::ostream &operator<<(std::ostream &o, const vector_time &msg)
{
    internal::vector_time int_time;
    int_time.set_index(msg._idx);
    auto vec = int_time.mutable_vector();
    vec->Resize(msg._time.size(), 0);
    std::copy(msg._time.cbegin(), msg._time.cend(), vec->begin());
    int_time.SerializeToOstream(&o);
    return o;
}

/*!
 * \brief Десериализует векторное время из потока
 */
std::istream &operator>>(std::istream &i, vector_time &msg)
{
    internal::vector_time int_time;
    int_time.ParseFromIstream(&i);
    msg._idx = int_time.index();
    auto vec = int_time.vector();
    msg._time.resize(int_time.vector_size());
    std::copy(vec.cbegin(), vec.cend(), msg._time.begin());
    return i;
}

} // namespace ultra
