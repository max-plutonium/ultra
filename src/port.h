#ifndef PORT_H
#define PORT_H

#include <sstream>
#include <memory>

#include "logic_time.h"

namespace ultra {

class port_message;

enum class openmode : int {
    app = std::ios_base::app,
    ate = std::ios_base::ate,
    binary = std::ios_base::binary,
    in = std::ios_base::in,
    out = std::ios_base::out,
    trunc = std::ios_base::trunc,
    inout = std::ios_base::in | std::ios_base::out
};

class port : public std::stringstream
{
    class impl;
    std::shared_ptr<impl> _impl;
    scalar_time _time;

public:
    explicit port(ultra::openmode om = ultra::openmode::inout);
    virtual ~port();

    scalar_time get_time() const { return _time; }
    ultra::openmode open_mode() const;
    bool connect(const port &);
    void disconnect(const port &);

protected:
    explicit port(std::shared_ptr<impl> &&d, ultra::openmode om = ultra::openmode::inout);
    friend class port_message;
    friend class vm;
    friend class reaction;
};

using port_ptr = std::shared_ptr<port>;

} // namespace ultra

#endif // PORT_H
