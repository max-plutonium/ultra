#ifndef ADDRESS_H
#define ADDRESS_H

#include <initializer_list>
#include <iostream>
#include "ultra_global.h"

namespace ultra {

    struct address
    {
        constexpr address() noexcept = default;
        address(int cluster, int space, int field, int node) noexcept;
        address(const std::initializer_list<int> &il);

        bool operator==(const address &o) const;
        bool operator!=(const address &o) const;

        inline int cluster() const { return _cluster; }
        inline int space() const { return _space; }
        inline int field() const { return _field; }
        inline int node() const { return _node; }
        inline void set_cluster(int acluster) { _cluster = acluster; }
        inline void set_space(int aspace) { _space = aspace; }
        inline void set_field(int afield) { _field = afield; }
        inline void set_node(int anode) { _node = anode; }

    private:
        int _cluster = 0, _space = 0, _field = 0, _node = 0;

        friend std::ostream &operator<<(std::ostream &o, const address &msg);
        friend std::istream &operator>>(std::istream &i, address &msg);
    };

    std::ostream &operator<<(std::ostream &o, const address &msg);
    std::istream &operator>>(std::istream &i, address &msg);

    struct address_hash
    {
        std::size_t operator()(const address &c) const;
    };

} // namespace ultra

#endif // ADDRESS_H
