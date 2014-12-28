#ifndef ADDRESS_H
#define ADDRESS_H

#include <initializer_list>
#include <iostream>
#include "ultra_global.h"

namespace ultra
{
    struct address
    {
        constexpr address() noexcept = default;
        address(int ax, int ay, int az) noexcept;
        address(const std::initializer_list<int> &il);

        bool operator==(const address &o) const;
        bool operator!=(const address &o) const;

        inline int x() const { return _x; }
        inline int y() const { return _y; }
        inline int z() const { return _z; }
        inline void set_x(int ax) { _x = ax; }
        inline void set_y(int ay) { _y = ay; }
        inline void set_z(int az) { _z = az; }

    private:
        int _x = 0, _y = 0, _z = 0;

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
