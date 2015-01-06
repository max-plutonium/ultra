#ifndef PORT_H
#define PORT_H

#include <sstream>
#include <memory>
#include "node.h"

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

class port : public node, public std::stringstream
{
    class impl;
    std::shared_ptr<impl> _impl;

public:
    explicit port(ultra::openmode om = ultra::openmode::inout,
                  const address &a = address(),
                  node *parent = nullptr);
    virtual ~port();

    ultra::openmode open_mode() const;
    bool connect(const port &);
    void disconnect(const port &);

protected:
    void post_message(const std::string &data = "");
    friend class port_message;
    friend class vm;
};

} // namespace ultra

#endif // PORT_H
