#include "port.h"
#include "core/core_p.h"

namespace ultra {

port::port(ultra::openmode om, const address &a, node *parent)
    : node(a, parent)
    , std::stringstream(static_cast<std::ios_base::openmode>(om))
    , _impl(std::make_shared<impl>(this, om))
{
    std::iostream::rdbuf(_impl.get());
    this->init(_impl.get());
}

port::~port()
{
    _impl->disconnect_all_senders();
    _impl->disconnect_all_receivers();
}

openmode port::open_mode() const
{
    return _impl->_om;
}

bool port::connect(const port &areceiver)
{
    if(!_impl->connect_receiver(areceiver._impl))
        return false;

    _time.advance();
    port_message msg(port_message::connect_sender, _impl,
                areceiver._impl, _time);
    vm::instance()->post_message(std::move(msg));
    return true;
}

void port::disconnect(const port &areceiver)
{
    _impl->disconnect_receiver(areceiver._impl);

    _time.advance();
    port_message msg(port_message::disconnect_sender, _impl,
                areceiver._impl, _time);
    vm::instance()->post_message(std::move(msg));
}

} // namespace ultra
