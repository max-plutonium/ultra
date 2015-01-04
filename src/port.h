#ifndef NODE_H
#define NODE_H

#include "address.h"
#include "logic_time.h"
#include "message.h"
#include <sstream>
#include <deque>

namespace ultra {

enum class openmode : int {
    app = std::ios_base::app,
    ate = std::ios_base::ate,
    binary = std::ios_base::binary,
    in = std::ios_base::in,
    out = std::ios_base::out,
    trunc = std::ios_base::trunc,
    inout = std::ios_base::in | std::ios_base::out
};

class port : public std::stringstream, public std::enable_shared_from_this<port>
{
    address _addr;
    scalar_time _time;
    ultra::openmode _om;

    class notifier;
    notifier *_notifier;

    std::deque<std::weak_ptr<port>> _senders, _receivers;

public:
    explicit port(address a, ultra::openmode om);
    virtual ~port();

    address get_address() const;
    scalar_time time() const;
    ultra::openmode open_mode() const;
    bool connect(const std::shared_ptr<port> &);
    void disconnect(const std::shared_ptr<port> &);

protected:
    virtual void message(const scalar_message_ptr &);
    void post_message(scalar_message::msg_type type, const std::string &data = "");
    friend class vm;

    bool connect_sender(const std::shared_ptr<port> &asender);
    bool connect_receiver(const std::shared_ptr<port> &areceiver);

    void disconnect_sender(const std::shared_ptr<port> &asender);
    void disconnect_receiver(const std::shared_ptr<port> &areceiver);

    void disconnect_all_senders();
    void disconnect_all_receivers();
};

using port_ptr = std::shared_ptr<port>;

} // namespace ultra

#endif // NODE_H
