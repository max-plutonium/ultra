#ifndef ADDRESS_H
#define ADDRESS_H

#include <initializer_list>
#include <iostream>

#include "ultra_global.h"

namespace ultra {

    /*!
     * \brief Адрес, используемый для поиска и доступа к элементам библиотеки
     */
    struct address
    {
        constexpr address() noexcept = default;
        address(int cluster, int space, int field, int node) noexcept;
        address(const std::initializer_list<int> &il);

        bool operator==(const address &o) const;
        bool operator!=(const address &o) const;

        int cluster() const;
        int space() const;
        int field() const;
        int node() const;
        void set_cluster(int acluster);
        void set_space(int aspace);
        void set_field(int afield);
        void set_node(int anode);

    private:
        int _cluster = 0, _space = 0, _field = 0, _node = 0;

        friend std::ostream &operator<<(std::ostream &o, const address &a);
        friend std::istream &operator>>(std::istream &i, address &a);
    };

    std::ostream &operator<<(std::ostream &o, const address &a);
    std::istream &operator>>(std::istream &i, address &a);

    /*!
     * \brief Функтор для хеширования объектов класса \c address
     */
    struct address_hash
    {
        std::size_t operator()(const address &c) const;
    };

} // namespace ultra

#endif // ADDRESS_H
