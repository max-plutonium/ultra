#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <iostream>
#include <memory>
#include "logic_time.h"
#include "address.h"

namespace ultra {

class scalar_message
{
    scalar_time _time;
    address _sender, _receiver;
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
    scalar_message() = default;
    scalar_message(msg_type type, address sender, address receiver,
                   scalar_time t = scalar_time(), const std::string &data = "");
    scalar_time time() const;
    address sender() const;
    address receiver() const;
    msg_type type() const;
    std::string data() const;

    bool operator==(const scalar_message &o) const;
    bool operator!=(const scalar_message &o) const;

    friend std::ostream &operator<<(std::ostream &o, const scalar_message &msg);
    friend std::istream &operator>>(std::istream &i, scalar_message &msg);
};

using scalar_message_ptr = std::shared_ptr<scalar_message>;

std::ostream &operator<<(std::ostream &o, const scalar_message &msg);
std::istream &operator>>(std::istream &i, scalar_message &msg);

} // namespace ultra

#endif // MESSAGE_H
