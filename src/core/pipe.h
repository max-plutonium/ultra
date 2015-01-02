#ifndef PIPE_H
#define PIPE_H

#include <ios>

namespace ultra { namespace core {

class pipe
{
    struct impl;
    impl *const d;

public:
    pipe();
    ~pipe();
    void add_reader(std::istream *);
    void add_writer(std::ostream *);
};

} // namespace core

} // namespace ultra

#endif // PIPE_H
