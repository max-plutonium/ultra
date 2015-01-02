#ifndef NODE_H
#define NODE_H

#include "address.h"
#include "logic_time.h"
#include "message.h"
#include <atomic>
#include <chrono>

namespace ultra {

class edge;

class node
{
    address _addr;
    scalar_time _time;
    edge *_senders, *_receivers;

public:
    explicit node(address a);
    virtual ~node();

    address get_address() const;
    scalar_time time() const;
    bool connect(const std::shared_ptr<node> &);
    bool disconnect(const std::shared_ptr<node> &);

protected:
    virtual void message(scalar_message_ptr);

public:
    void post_message(scalar_message::msg_type type,
                      address addr, const std::string &data = "");

private:
    bool connect_sender(node *asender);
    bool connect_receiver(node *areceiver);
    void connect_sender(edge *c);
    void connect_receiver(edge *c);
    bool disconnect_sender(node *asender);
    bool disconnect_receiver(node *areceiver);
    void disconnect_all_senders();
    void disconnect_all_receivers();

    friend class vm;
};

using node_ptr = std::shared_ptr<node>;

class edge
{
    node *_sender;
    std::atomic<node *> _receiver;
    edge *_next, *_down;

public:
    edge(node *sender, node *receiver);
    friend class node;
};

} // namespace ultra

#endif // NODE_H
