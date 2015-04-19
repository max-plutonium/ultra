#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <iostream>
#include <memory>

#include "logic_time.h"
#include "port.h"

namespace ultra {

class request
{
public:
    enum command_type {
        ping = 0,
        input_data = 1,
        debug_start = 2,
        debug_stop = 3
    };

private:
    command_type _type;
    std::string _data;

public:
    request() = default;
    explicit request(command_type type, const std::string &data = std::string());
    command_type type() const;
    std::string data() const;

    friend std::ostream &operator<<(std::ostream &o, const request &msg);
    friend std::istream &operator>>(std::istream &i, request &msg);
};

std::ostream &operator<<(std::ostream &o, const request &msg);
std::istream &operator>>(std::istream &i, request &msg);

class reply
{
public:
    enum command_type {
        pong = 0,
        output_data = 1
    };

private:
    command_type _type;
    std::string _data;

public:
    reply() = default;
    explicit reply(command_type type, const std::string &data = std::string());
    command_type type() const;
    std::string data() const;

    friend std::ostream &operator<<(std::ostream &o, const reply &msg);
    friend std::istream &operator>>(std::istream &i, reply &msg);
};

std::ostream &operator<<(std::ostream &o, const reply &msg);
std::istream &operator>>(std::istream &i, reply &msg);

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
};

} // namespace ultra

#endif // MESSAGE_H
