#ifndef ADDRESS_H
#define ADDRESS_H

#include <initializer_list>
#include "ultra_global.h"
#include <initializer_list>

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
    };


    struct address_hash
    {
        std::size_t operator()(const address &c) const;
    };

} // namespace ultra

#endif // ADDRESS_H
