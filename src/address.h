#ifndef ADDRESS_H
#define ADDRESS_H

#include "ultra_global.h"

namespace ultra ULTRA_EXPORT
{
    struct ULTRA_EXPORT address
    {
        constexpr address() noexcept : _x(0), _y(0), _z(0) { }

        address(int ax, int ay, int az) : _x(ax), _y(ay), _z(az) { }

        address(const std::initializer_list<int> &il)
            : _x(il.begin()[0]), _y(il.begin()[1]), _z(il.begin()[2]) { }

        bool operator==(const address &o) const
        { return (_x == o._x && _y == o._y && _z == o._z); }
        bool operator!=(const address &o) const
        { return !operator==(o); }
        int x() const { return _x; }
        int y() const { return _y; }
        int z() const { return _z; }
        void set_x(int ax) { _x = ax; }
        void set_y(int ay) { _y = ay; }
        void set_z(int az) { _z = az; }

    private:
        int _x, _y, _z;
    };


    struct ULTRA_EXPORT address_hash
    {
        std::size_t operator()(const address &c) const;
    };

} // namespace ultra

#endif // ADDRESS_H
