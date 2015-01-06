#include "address.h"
#include <boost/functional/hash.hpp>
#include "msg.pb.h"

namespace ultra {

/*!
 * \brief Конструктор
 *
 * \param cluster Номер кластера.
 * \param space Номер оперативного пространства.
 * \param field Номер оперативного поля.
 * \param node Номер узла.
 */
address::address(int cluster, int space, int field, int node) noexcept
    : _cluster(cluster), _space(space), _field(field), _node(node)
{ }

/*!
 * \brief Конструирует объект из \c initializer_list
 */
address::address(const std::initializer_list<int> &il)
    : _cluster(il.begin()[0]), _space(il.begin()[1])
    , _field(il.begin()[2]), _node(il.begin()[3])
{ }

/*!
 * \brief Сравнивает два адреса между собой
 *
 * \return true, если они равны.
 */
bool address::operator==(const address &o) const
{
    return (_cluster == o._cluster && _space == o._space
            && _field == o._field && _node == o._node);
}

/*!
 * \brief Сравнивает два адреса между собой
 *
 * \return true, если они не равны.
 */
bool address::operator!=(const address &o) const
{
    return !operator==(o);
}

/*!
 * \brief Возвращает номер кластера
 */
int address::cluster() const
{
    return _cluster;
}

/*!
 * \brief Возвращает номер оперативного пространства
 */
int address::space() const
{
    return _space;
}

/*!
 * \brief Возвращает номер оперативного поля
 */
int address::field() const
{
    return _field;
}

/*!
 * \brief Возвращает номер узла
 */
int address::node() const
{
    return _node;
}

/*!
 * \brief Устанавливает номер кластера из \a acluster
 */
void address::set_cluster(int acluster)
{
    _cluster = acluster;
}

/*!
 * \brief Устанавливает номер оперативного пространста из \a aspace
 */
void address::set_space(int aspace)
{
    _space = aspace;
}

/*!
 * \brief Устанавливает номер оперативного поля из \a afield
 */
void address::set_field(int afield)
{
    _field = afield;
}

/*!
 * \brief Устанавливает номер узла из \a anode
 */
void address::set_node(int anode)
{
    _node = anode;
}

/*!
 * \brief Возвращает хеш для объекта адреса \a c
 */
std::size_t address_hash::operator()(const address &c) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, c.cluster());
    boost::hash_combine(seed, c.space());
    boost::hash_combine(seed, c.field());
    boost::hash_combine(seed, c.node());
    return seed;
}

/*!
 * \brief Выводит объект адреса \a a в поток \a o
 */
std::ostream &operator<<(std::ostream &o, const address &a)
{
    internal::address int_address;
    int_address.set_cluster(a.cluster());
    int_address.set_space(a.space());
    int_address.set_field(a.field());
    int_address.set_node(a.node());
    int_address.SerializeToOstream(&o);
    return o;
}

/*!
 * \brief Вводит объект адреса \a a из потока \a i
 */
std::istream &operator>>(std::istream &i, address &a)
{
    internal::address int_address;
    int_address.ParseFromIstream(&i);
    a._cluster = int_address.cluster();
    a._space = int_address.space();
    a._field = int_address.field();
    a._node = int_address.node();
    return i;
}

} // namespace ultra
