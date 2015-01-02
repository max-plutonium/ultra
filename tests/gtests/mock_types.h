#ifndef MOCK_TYPES_H
#define MOCK_TYPES_H

#include <cstdint>
#include <gmock/gmock.h>


template <bool NoexceptMovable = true>
class copyable_movable_t
{
    int i;
    bool copied = false, moved = false;

public:
    copyable_movable_t(int _i) : i(_i) { }

    copyable_movable_t(const copyable_movable_t &o) : copyable_movable_t(o.i) { copied = true; }
    copyable_movable_t &operator=(const copyable_movable_t &o) { i = o.i; copied = true; return *this; }

    copyable_movable_t(copyable_movable_t &&o) noexcept(NoexceptMovable) : copyable_movable_t(o.i) { o.i = 0; moved = true; }
    copyable_movable_t &operator=(copyable_movable_t &&o) noexcept(NoexceptMovable) { i = o.i; o.i = 0; moved = true; return *this; }

    ~copyable_movable_t() { i = -1; }

    int get() const { return i; }
    bool was_copied() const { return copied; }
    bool was_moved() const { return moved; }
};

class copyable_but_not_movable_t
{
    int i;
    bool copied = false, moved = false;

public:
    copyable_but_not_movable_t(int _i) : i(_i) { }

    copyable_but_not_movable_t(const copyable_but_not_movable_t &o) : copyable_but_not_movable_t(o.i) { copied = true; }
    copyable_but_not_movable_t &operator=(const copyable_but_not_movable_t &o) { i = o.i; copied = true; return *this; }

    copyable_but_not_movable_t(copyable_but_not_movable_t &&o) = delete;
    copyable_but_not_movable_t &operator=(copyable_but_not_movable_t &&o) = delete;

    ~copyable_but_not_movable_t() { i = -1; }

    int get() const { return i; }
    bool was_copied() const { return copied; }
    bool was_moved() const { return moved; }
};

class not_copyable_but_movable_t
{
    int i;
    bool copied = false, moved = false;

public:
    not_copyable_but_movable_t(int _i) : i(_i) { }

    not_copyable_but_movable_t(const not_copyable_but_movable_t &o) = delete;
    not_copyable_but_movable_t &operator=(const not_copyable_but_movable_t &o) = delete;

    not_copyable_but_movable_t(not_copyable_but_movable_t &&o) noexcept : not_copyable_but_movable_t(o.i) { o.i = 0; moved = true; }
    not_copyable_but_movable_t &operator=(not_copyable_but_movable_t &&o) noexcept { i = o.i; o.i = 0; moved = true; return *this; }

    ~not_copyable_but_movable_t() { i = -1; }

    int get() const { return i; }
    bool was_copied() const { return copied; }
    bool was_moved() const { return moved; }
};

class not_copyable_not_movable_t
{
    int i;
    bool copied = false, moved = false;

public:
    not_copyable_not_movable_t(int _i) : i(_i) { }

    not_copyable_not_movable_t(const not_copyable_not_movable_t &o) = delete;
    not_copyable_not_movable_t &operator=(const not_copyable_not_movable_t &o) = delete;

    not_copyable_not_movable_t(not_copyable_not_movable_t &&o) = delete;
    not_copyable_not_movable_t &operator=(not_copyable_not_movable_t &&o) = delete;

    ~not_copyable_not_movable_t() { i = -1; }

    int get() const { return i; }
    bool was_copied() const { return copied; }
    bool was_moved() const { return moved; }
};


struct dummy_mutex
{
    bool locked = false;
    void lock() { locked = true; }
    void unlock() { locked = false; }
};

class mock_mutex
{
public:
    mock_mutex() = default;
    mock_mutex(const mock_mutex&) = delete;
    mock_mutex &operator=(const mock_mutex&) = delete;
    mock_mutex(mock_mutex&&) noexcept = default;
    mock_mutex &operator=(mock_mutex&&) noexcept = default;
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};

#include "../../src/task.h"

class mock_task : public ultra::task
{
public:
    explicit mock_task(int prio = 0) : ultra::task(prio) { }
    MOCK_METHOD0(run, void());
};

#include "../../src/node.h"

class mock_node : public ultra::node
{
public:
    mock_node(ultra::address a) : ultra::node(a) { }
    MOCK_METHOD1(message, void (ultra::scalar_message_ptr));
};

#endif // MOCK_TYPES_H
