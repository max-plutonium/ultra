#ifndef VM_H
#define VM_H

#include <atomic>
#include <memory>
#include <cstring>
#include "ultra_global.h"
#include "address.h"

namespace ultra {

struct scalar_time {
    std::size_t _time;
};



class edge;

class node {
    address _addr;

    // Список соединений, где обьект является приемником
    edge *_senders = nullptr;

    // Список соединений, где обьект является посылающим
    edge *_receivers = nullptr;

protected:


public:
};

class edge {
    node *_sender;
    std::atomic<node *> _receiver;
    edge *_down, *_next;

    edge(node *sender, node *receiver)
        : _sender(sender), _receiver(receiver)
        , _down(nullptr), _next(nullptr)
    { }

  template <typename... Args>
    static inline edge*
    create(Args&&... args) {
        return new edge(std::forward<Args>(args)...);
    }

    static inline void
    destroy(edge *ptr) {
        delete ptr;
    }
};

class vm
{
protected:
    vm() = default;
    virtual ~vm() = default;

public:
    static std::shared_ptr<vm> create_vm(int argc, char **argv);
};

} // namespace ultra

#endif // VM_H
