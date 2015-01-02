#include "pipe.h"
#include <iostream>
#include "core/concurrent_queue.h"
#include <vector>
#include <cstring>

namespace ultra { namespace core {

struct pipe::impl : public std::streambuf
{
    core::concurrent_queue<std::vector<char>, std::mutex> _packages;

    // basic_streambuf interface
protected:
    virtual std::streamsize xsgetn(char_type *s, std::streamsize n) override;
    virtual std::streamsize xsputn(const char_type *s, std::streamsize n) override;
};

std::streamsize pipe::impl::xsgetn(char_type *s, std::streamsize /*n*/)
{
    std::vector<char> vec;
    if(!_packages.dequeue(vec))
        return 0;
    std::memmove(s, vec.data(), vec.size());
    return vec.size();
}

std::streamsize pipe::impl::xsputn(const char_type *s, std::streamsize n)
{
    _packages.enqueue(s, s + n);
    return n;
}


/************************************************************************************
    pipe
 ***********************************************************************************/
pipe::pipe() : d(new impl)
{
}

pipe::~pipe()
{
    delete d;
}

void pipe::add_reader(std::istream *istream)
{
    istream->rdbuf(d);
}

void pipe::add_writer(std::ostream *ostream)
{
    ostream->rdbuf(d);
}

} // namespace core

} // namespace ultra

