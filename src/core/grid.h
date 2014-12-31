#ifndef GRID_H
#define GRID_H

#include <unordered_map>
#include <utility>
#include <iostream>

namespace ultra { namespace core {

template <typename X, typename Y, typename Z,
          typename KeyXHasher    = std::hash<X>,
          typename KeyYHasher    = std::hash<Y>,
          typename KeyXPredicate = std::equal_to<X>,
          typename KeyYPredicate = std::equal_to<Y>,
          typename KeyXCompare   = std::less<X>,
          typename KeyYCompare   = std::less<Y>,
          typename Allocator     = std::allocator<X> >
class grid
{
    struct node
    {
        X _x; Y _y; Z _z;
        node **_up, *_down, **_prev, *_next;

        node(X x, Y y, Z z = Z()) : _x(x), _y(y), _z(z)
          , _up(0), _down(0), _prev(0), _next(0) { }

        ~node() {
            if(_down) _down->_up = _up;
            if(_up)   *_up = _down;
            if(_next) _next->_prev = _prev;
            if(_prev) *_prev = _next;
        }

      template <typename... Args>
        static inline node *create(Args &&...args) {
            typename Allocator::template rebind<node>::other a;
            node *ptr = a.allocate(1);
            a.construct(ptr, std::forward<Args>(args)...);
            return ptr;
        }

        static inline void destroy(node *ptr) {
            typename Allocator::template rebind<node>::other a;
            a.destroy(ptr);
            a.deallocate(ptr, 1);
        }
    };

    struct connections_x
    {
        node *_down = nullptr;
        ~connections_x() {
            for(register node *c = _down; c; c = _down)
                node::destroy(c);
        }
    };

    struct connections_y
    {
        node *_next = nullptr;
        ~connections_y() {
            for(register node *c = _next; c; c = _next)
                node::destroy(c);
        }
    };

    using mapx = std::unordered_map
        <X, connections_x, KeyXHasher, KeyXPredicate, Allocator>;
    using mapy = std::unordered_map
        <Y, connections_y, KeyYHasher, KeyYPredicate, Allocator>;

public:
    typedef X x_key_type;
    typedef Y y_key_type;
    typedef KeyXHasher      x_hasher;
    typedef KeyYHasher      y_hasher;
    typedef KeyXPredicate   x_key_equal;
    typedef KeyYPredicate   y_key_equal;
    typedef KeyXCompare     x_key_compare;
    typedef KeyYCompare     y_key_compare;
    typedef Allocator       allocator_type;
    typedef std::size_t     size_type;

    void insert_x(const X &x) {
        _mapx.emplace(x, connections_x());
    }

    void insert_y(const Y &y) {
        _mapy.emplace(y, connections_y());
    }

    inline bool contains_x(const X &x) const
    { return _mapx.find(x) != _mapx.end(); }

    inline bool contains_y(const Y &y) const
    { return _mapy.find(y) != _mapy.end(); }

    bool remove_x(const X &x) {
        typename mapx::iterator it = _mapx.find(x);
        if(it == _mapx.end())
            return false;
        _mapx.erase(it);
        return true;
    }

    bool remove_y(const Y &y) {
        typename mapy::iterator it = _mapy.find(y);
        if(it == _mapy.end())
            return false;
        _mapy.erase(it);
        return true;
    }

    bool connect(const X &x, const Y &y) {
        typename mapx::iterator itx = _mapx.find(x);
        typename mapy::iterator ity = _mapy.find(y);
        if(itx == _mapx.end() || ity == _mapy.end())
            return false;

        register node *cur = itx->second._down;
        node **down_ptr = &itx->second._down,
                        **next_ptr = &ity->second._next;
        if(cur) {
            KeyYCompare compy;
            KeyYPredicate predy;
            while(compy(cur->_y, y)) {
                if(!cur->_down) {
                    down_ptr = &cur->_down;
                    cur = cur->_down;
                    break;
                }
                cur = cur->_down;
            }
            if(cur) {
                if(predy(cur->_y, y))
                    return false;
                else
                    down_ptr = cur->_up;
            }
        }

        node *c = node::create(x, y);
        c->_up   = down_ptr;
        c->_down = *c->_up;
        *c->_up  = c;
        if(c->_down)
            c->_down->_up = &c->_down;

        c->_prev = next_ptr;
        c->_next = *c->_prev;
        *c->_prev  = c;
        if(c->_next)
            c->_next->_prev = &c->_next;

        return true;
    }

    bool disconnect(const X &x, const Y &y) {
        typename mapx::iterator itx = _mapx.find(x);
        if(itx == _mapx.end())
            return false;
        register node *cur = itx->second._down;

        if(cur) {
            KeyYCompare compy;
            KeyYPredicate predy;
            while(compy(cur->_y, y)) {
                cur = cur->_down;
                if(!cur) break;
            }
            if(cur && predy(cur->_y, y)) {
                node::destroy(cur);
                return true;
            }
        }

        return false;
    }

    bool connected(const X &x, const Y &y) const {
        typename mapx::const_iterator it = _mapx.find(x);
        if(it == _mapx.end())
            return false;
        register node *cur = it->second._down;

        if(cur) {
            KeyYCompare compy;
            KeyYPredicate predy;
            while(compy(cur->_y, y)) {
                cur = cur->_down;
                if(!cur) break;
            }
            if(cur && predy(cur->_y, y))
                return true;
        }
        return false;
    }

    inline size_type size_x() const { return _mapx.size(); }
    inline size_type size_y() const { return _mapy.size(); }

    void dump()
    {
        for(auto &itx : _mapx)
        {
            std::cout << itx.first << "->   ";
            for(auto *c = itx.second._down;
                c; c = c->_down)
            {
                std::cout << c->_y << "-    ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        for(auto &ity : _mapy)
        {
            std::cout << ity.first << "->   ";
            for(auto *c = ity.second._next;
                c; c = c->_next)
            {
                std::cout << c->_x << "-    ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    class iterator : std::iterator<std::forward_iterator_tag, std::pair<X, Y> >
    {
    protected:
        node *i = nullptr;

    public:
        inline iterator() = default;
        inline iterator(node *p) : i(p) {}
        inline iterator(const iterator&) = default;
        inline iterator &operator=(const iterator&) = default;
        inline iterator(const iterator &&o) noexcept { swap(o); }
        inline iterator &operator=(iterator &&o) noexcept
        { iterator(std::move(o)).swap(*this); }
        inline ~iterator() = default;
        inline void swap(iterator &o) noexcept { std::swap(i, o.i); }

        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }
        inline std::pair<X, Y> operator*() const
        { return std::make_pair(i->_x, i->_y); }
        inline const node *operator->() const { return i; }
    };

    class x_iterator : public iterator
    {
        using iterator::i;

    public:
        inline x_iterator() = default;
        inline x_iterator(node *p) : iterator(p) {}
        inline x_iterator(const x_iterator&) = default;
        inline x_iterator &operator=(const x_iterator&) = default;

        using iterator::swap;
        using iterator::operator==;
        using iterator::operator!=;
        using iterator::operator*;
        using iterator::operator->;
        inline x_iterator &operator++() { i = i->_down; return *this; }
        inline x_iterator operator++(int)
        { x_iterator it = *this; i = i->_down; return it; }
    };

    class y_iterator : public iterator
    {
        using iterator::i;

    public:
        inline y_iterator() = default;
        inline y_iterator(node *p) : iterator(p) {}
        inline y_iterator(const y_iterator&) = default;
        inline y_iterator &operator=(const y_iterator&) = default;

        using iterator::swap;
        using iterator::operator==;
        using iterator::operator!=;
        using iterator::operator*;
        using iterator::operator->;
        inline y_iterator &operator++() { i = i->_next; return *this; }
        inline y_iterator operator++(int)
        { y_iterator it = *this; i = i->_next; return it; }
    };

    friend class iterator;
    friend class x_iterator;
    friend class y_iterator;

    inline x_iterator xbegin(const X &x) const
    { return x_iterator(_mapx.at(x)._down); }
    inline x_iterator xend() const { return x_iterator(nullptr); }
    inline y_iterator ybegin(const Y &y) const
    { return y_iterator(_mapy.at(y)._next); }
    inline y_iterator yend() const { return y_iterator(nullptr); }

private:
    mapx _mapx;
    mapy _mapy;
};

} // namespace core

} // namespace ultra

#endif // GRID_H
