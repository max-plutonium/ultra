#include "node.h"
#include "vm.h"
#include <cassert>
#include <sstream>

#include "core/action.h"

#define ULTRA_SIGNAL_TRACE

namespace ultra {

class function_tracer
{

/*! \internal
 *  \brief  Непосредственно выводит трассировочное сообщение
 *  \param  amessage  Форматная строка для выводимого сообщения.
 *  \param  atuple    Кортеж с аргументами форматной строки.
 *  \note   Кол-во типов в \c TupleArgs должно совпадать с
 *  кол-вом индексов в \c Indices.
 */
  template <typename... TupleArgs, size_t... Indices>
    inline static void s_trace_message_impl(const char *amessage,
        std::tuple<TupleArgs...> &atuple, core::details::index_sequence<Indices...>)
    {
        fprintf(stderr, amessage,
                std::forward<TupleArgs>(std::get<Indices>(atuple))...);
    }

    struct function_tracer_impl_base {
        virtual ~function_tracer_impl_base() {}
    };

    std::unique_ptr<function_tracer_impl_base> tracer_impl;

  template <typename... Args>
    class function_tracer_impl : public function_tracer_impl_base
    {
        // Форматная строка
        std::string m_str;

        // Кортеж аргументов форматной строки. Глупо хранить
        // здесь ссылки на что-либо, поэтому перед распаковкой
        // типов в шаблон кортежа снимаем с них ссылочный тип
        std::tuple<typename
            std::remove_reference<Args>::type...> m_tuple;

    public:
        function_tracer_impl(std::string astr, Args&&... args)
            : m_str(astr)
            , m_tuple(std::make_tuple(std::forward<Args>(args)...))
        {
            s_trace_message_impl(
                std::string("--> " + m_str).data(), m_tuple,
                    core::details::make_index_sequence_for<Args...>() );
        }

        virtual ~function_tracer_impl() override
        {
            s_trace_message_impl(
                std::string("<-- " + m_str).data(), m_tuple,
                    core::details::make_index_sequence_for<Args...>() );
        }

    };  //  class function_tracer_impl

protected:
    function_tracer() = default;

public:
  template <typename... Args>
    function_tracer(std::string astr, Args&&... args)
        : tracer_impl(new function_tracer_impl<Args...>(
                          astr, std::forward<Args>(args)...) )
    {}

    ~function_tracer() = default;

  template <typename... Args>
    void message(std::string astr, Args&&... args)
    {
        auto tpl = std::make_tuple(std::forward<Args>(args)...);
        s_trace_message_impl(
            std::string("*** " + astr).data(), tpl,
                core::details::make_index_sequence_for<Args...>() );
    }

};  //  class function_tracer


/************************************************************************************
    node
 ***********************************************************************************/
node::node(address a)
    : _addr(a), _senders(nullptr), _receivers(nullptr)
{
}

node::~node()
{
    disconnect_all_senders();
    disconnect_all_receivers();
    assert(!_senders && !_receivers);
}

address node::get_address() const
{
    return _addr;
}

scalar_time node::time() const
{
    return _time;
}

bool node::connect(const std::shared_ptr<node> &other)
{
    return connect_receiver(other.get());
}

bool node::disconnect(const std::shared_ptr<node> &other)
{
    return disconnect_receiver(other.get());
}

void node::message(scalar_message_ptr msg)
{
    _time.merge(msg->time());
    _time.advance();

    std::intptr_t data_as_ptr = 0;

    if(!msg->data().substr(0, 2).compare("0x")) {
        std::stringstream ss;
        ss << msg->data();
        ss >> std::hex >> data_as_ptr;
    }

    switch(msg->type()) {

        case scalar_message::connect_sender:
            connect_sender(reinterpret_cast<edge *>(data_as_ptr));
            break;

        case scalar_message::connect_receiver:
            connect_receiver(reinterpret_cast<edge *>(data_as_ptr));
            break;

        case scalar_message::disconnect_sender:
            disconnect_sender(reinterpret_cast<node *>(data_as_ptr));
            break;

        case scalar_message::disconnect_receiver:
            disconnect_receiver(reinterpret_cast<node *>(data_as_ptr));
            break;

        default:
            break;
    }
}

void node::post_message(scalar_message::msg_type type,
                        address addr, const std::string &data)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::post(signal &s): "
                           "s = %i, this = %p, thread = %p\n",
                           s.operator sigtype(), this, pthread_self());
#endif

    _time.advance();
    auto msg = std::make_shared<scalar_message>(type, _addr, addr, _time, data);

    edge *cur = _receivers, *before_cur = nullptr;

    while(cur) {
        node *curreceiver
                = cur->_receiver.load(std::memory_order_acquire);

        if(ULTRA_EXPECT(curreceiver, true)) {
            vm::instance()->post_message(msg);
            before_cur = cur;
            cur = cur->_down;

        } else {
            if(before_cur)
                before_cur->_down = cur->_down;
            else
                _receivers = cur->_down;
            auto old_cur = cur;
            cur = cur->_down;
            delete old_cur;
        }
    }
}

bool node::connect_sender(node *asender)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::connect_sender(device_impl *asender): "
                           "asender = %p, this = %p, thread = %p\n",
                           asender, this, pthread_self());
#endif
    assert(asender);
    edge *cur = _senders;

    if(cur) {
        node *cursender = cur->_sender;
        edge *before_cur = nullptr;

        while(cursender < asender)
        {
            before_cur = cur;
            cur = cur->_next;
            if(!cur) break;
            cursender = cur->_sender;
        }
        if(cur && cursender == asender)
            return false;
        edge *c = new edge(asender, this);
        c->_next = cur;
        if(before_cur)
            before_cur->_next = c;
        else
            _senders = c;
        cur = c;
    }
    else
        _senders = cur = new edge(asender, this);

    _time.advance();

    std::string str_ptr;
    std::stringstream ss; ss << cur; ss >> str_ptr;
    auto msg = std::make_shared<scalar_message>(
        scalar_message::connect_receiver, _addr, asender->_addr, _time, str_ptr);
    vm::instance()->post_message(std::move(msg));
    return true;
}

bool node::connect_receiver(node *areceiver)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::connect_receiver(device_impl *areceiver): "
                           "areceiver = %p, this = %p, thread = %p\n",
                           areceiver, this, pthread_self());
#endif

    assert(areceiver);
    edge *cur = _receivers;

    if(cur) {
        node *curreceiver
                = cur->_receiver.load(std::memory_order_acquire);
        edge *before_cur = nullptr;

        while(curreceiver < areceiver)
        {
            before_cur = cur;
            cur = cur->_down;
            if(!cur) break;
            curreceiver = cur->_receiver.load(std::memory_order_acquire);
        }
        if(cur && curreceiver == areceiver)
            return false;
        edge *c = new edge(this, areceiver);
        c->_down = cur;
        if(before_cur)
            before_cur->_down = c;
        else
            _receivers = c;
        cur = c;
    }
    else
        _receivers = cur = new edge(this, areceiver);

    _time.advance();

    std::string str_ptr;
    std::stringstream ss; ss << cur; ss >> str_ptr;
    auto msg = std::make_shared<scalar_message>(
        scalar_message::connect_sender, _addr, areceiver->_addr, _time, str_ptr);
    vm::instance()->post_message(std::move(msg));
    return true;
}

void node::connect_sender(edge *c)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::connect_sender(connection *c): "
                           "c = %p, this = %p, thread = %p\n",
                           c, this, pthread_self());
#endif

    assert(c);
    edge *cur = _senders;
    node *sender = c->_sender;
    assert(sender);

    if(cur) {
        node *cursender = cur->_sender;
        edge *before_cur = nullptr;

        while(cursender < sender)
        {
            before_cur = cur;
            cur = cur->_next;
            if(!cur) break;
            cursender = cur->_sender;
        }
        if(cursender != sender)
        {
            c->_next = cur;
            if(before_cur)
                before_cur->_next = c;
            else
                _senders = c;
        }
    }
    else
        _senders = c;
}

void node::connect_receiver(edge *c)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::connect_receiver(connection *c): "
                           "c = %p, this = %p, thread = %p\n",
                           c, this, pthread_self());
#endif

    assert(c);
    edge *cur = _receivers;
    node *receiver = c->_receiver.load(std::memory_order_relaxed);
    assert(receiver);

    if(cur) {
        node *curreceiver
                = cur->_receiver.load(std::memory_order_acquire);
        edge *before_cur = nullptr;

        while(curreceiver < receiver)
        {
            before_cur = cur;
            cur = cur->_down;
            if(!cur) break;
            curreceiver = cur->_receiver.load(std::memory_order_acquire);
        }
        if(curreceiver != receiver) {
            c->_down = cur;
            if(before_cur)
                before_cur->_down = c;
            else
                _receivers = c;
        }
    }
    else
        _receivers = c;
}

bool node::disconnect_sender(node *asender)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::disconnect_sender(device_impl *asender): "
                           "asender = %p, this = %p, thread = %p\n",
                           asender, this, pthread_self());
#endif

    assert(asender);
    edge *cur = _senders;

    if(cur)
    {
        node *cursender = cur->_sender;
        edge *before_cur = nullptr;

        while(cursender < asender)
        {
            before_cur = cur;
            cur = cur->_next;
            if(!cur) break;
            cursender = cur->_sender;
        }
        if(cur && cursender == asender)
        {
            if(before_cur)
                before_cur->_next = cur->_next;
            else
                _senders = cur->_next;

            auto expected = this;
            if(cur->_receiver.compare_exchange_strong(
                    expected, nullptr, std::memory_order_acq_rel))
            {
                _time.advance();

                std::string str_ptr;
                std::stringstream ss; ss << this; ss >> str_ptr;
                auto msg = std::make_shared<scalar_message>(
                    scalar_message::disconnect_sender, _addr, asender->_addr, _time, str_ptr);
                vm::instance()->post_message(std::move(msg));
            }
            else
                delete cur;
            return true;
        }
    }
    return false;
}

bool node::disconnect_receiver(node *areceiver)
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::disconnect_receiver(device_impl *areceiver): "
                           "areceiver = %p, this = %p, thread = %p\n",
                           areceiver, this, pthread_self());
#endif

    assert(areceiver);
    edge *cur = _receivers;

    if(cur)
    {
        node *curreceiver
                = cur->_receiver.load(std::memory_order_acquire);
        edge *before_cur = nullptr;

        while(curreceiver < areceiver)
        {
            before_cur = cur;
            cur = cur->_down;
            if(!cur) break;
            curreceiver = cur->_receiver.load(std::memory_order_acquire);
        }
        if(cur && curreceiver == areceiver)
        {
            if(before_cur)
                before_cur->_down = cur->_down;
            else
                _receivers = cur->_down;

            if(cur->_receiver.compare_exchange_strong(
                  curreceiver, nullptr, std::memory_order_acq_rel))
            {
                _time.advance();

                std::string str_ptr;
                std::stringstream ss; ss << this; ss >> str_ptr;
                auto msg = std::make_shared<scalar_message>(
                    scalar_message::disconnect_receiver, _addr, areceiver->_addr, _time, str_ptr);
                vm::instance()->post_message(std::move(msg));
            }
            else
                delete cur;

            return true;
        }
    }
    return false;
}

void node::disconnect_all_senders()
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::disconnect_all_senders(): "
                           "this = %p, thread = %p\n",
                           this, pthread_self());
#endif

    while(_senders) {
        auto expected = this;
        if(!_senders->_receiver.compare_exchange_strong(
               expected, nullptr, std::memory_order_acq_rel))
        {
            auto old_cur = _senders;
            _senders = _senders->_next;
            delete old_cur;
            continue;
        }
        _senders = _senders->_next;
    }
}

void node::disconnect_all_receivers()
{
#ifdef ULTRA_CORE_TRACE
    function_tracer tracer("device_impl::disconnect_all_receivers(): "
                           "this = %p, thread = %p\n",
                           this, pthread_self());
#endif

    while(_receivers) {
        node *curreceiver = _receivers->_receiver.exchange(nullptr, std::memory_order_acq_rel);

        if(!curreceiver) {
            auto old_cur = _receivers;
            _receivers = _receivers->_down;
            delete old_cur;

        } else {
            _time.advance();

            std::string str_ptr;
            std::stringstream ss; ss << this; ss >> str_ptr;
            auto msg = std::make_shared<scalar_message>(
                scalar_message::disconnect_sender, _addr, curreceiver->_addr, _time, str_ptr);
            vm::instance()->post_message(std::move(msg));
            _receivers = _receivers->_down;
        }
    }
}


/************************************************************************************
    edge
 ***********************************************************************************/
edge::edge(node *sender, node *receiver)
    : _sender(sender), _receiver(receiver)
    , _next(nullptr), _down(nullptr)
{
}

} // namespace ultra
