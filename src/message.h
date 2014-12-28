#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <iostream>
#include "logic_time.h"
#include "address.h"

namespace ultra {

struct scalar_message
{
    scalar_time _time;
    address _sender_address;
    std::string _data;

    explicit scalar_message(scalar_time t = scalar_time(),
                            address a = address(), const char *data = "");

    bool operator==(const scalar_message &o) const;
    bool operator!=(const scalar_message &o) const;

    friend std::ostream &operator<<(std::ostream &o, const scalar_message &msg);
    friend std::istream &operator>>(std::istream &i, scalar_message &msg);
};

std::ostream &operator<<(std::ostream &o, const scalar_message &msg);
std::istream &operator>>(std::istream &i, scalar_message &msg);

} // namespace ultra

#endif // MESSAGE_H
