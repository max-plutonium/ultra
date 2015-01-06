#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <iostream>
#include <memory>
#include "logic_time.h"
#include "port.h"

namespace ultra {

class port_message
{
    scalar_time _time;
    std::shared_ptr<port::impl> _sender;
    std::shared_ptr<port::impl> _receiver;
    std::string _data;

public:
    enum msg_type {
        unknown = 0,
        port_data = 1,
        connect_sender = 2,
        connect_receiver = 3,
        disconnect_sender = 4,
        disconnect_receiver = 5
    };

private:
    msg_type _type = unknown;

public:
    port_message() = default;
    port_message(msg_type type, std::shared_ptr<port::impl> sender,
            std::shared_ptr<port::impl> receiver,
            scalar_time t = scalar_time(),
            const std::string &data = "");
    scalar_time time() const;
    std::shared_ptr<port::impl> sender() const;
    std::shared_ptr<port::impl> receiver() const;
    msg_type type() const;
    std::string data() const;

    bool operator==(const port_message &o) const;
    bool operator!=(const port_message &o) const;

//    friend std::ostream &operator<<(std::ostream &o, const message &msg);
//    friend std::istream &operator>>(std::istream &i, message &msg);
};

using message_ptr = std::shared_ptr<port_message>;

//std::ostream &operator<<(std::ostream &o, const message &msg);
//std::istream &operator>>(std::istream &i, message &msg);

} // namespace ultra

#endif // MESSAGE_H
