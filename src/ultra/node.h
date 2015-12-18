/*
 * Copyright (C) 2015 Max Plutonium <plutonium.max@gmail.com>
 *
 * This file is part of the ULTRA library.
 *
 * The ULTRA library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * The ULTRA library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the ULTRA library. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef ULTRA_NODE_H
#define ULTRA_NODE_H

#include <boost/thread/shared_mutex.hpp>
#include <experimental/strand>
#include <unordered_map>

#include "message_handler.h"

namespace ultra {

using std::experimental::strand;
using node_ref = std::uint64_t;

class node
{
public:
    virtual ~node() = default;

    /// Obtain the actor's address for use as a message sender or recipient.
    node_ref address() const;

    /// Find the matching message handlers, if any, and call them.
  template <typename Message>
    void emit(Message msg)
    {
        std::vector<std::shared_ptr<message_handler<Message, node_ref>>> handlers;

        {
            boost::shared_lock<decltype(_lock)> lk(_lock);
            const auto pair = _map.equal_range(typeid(Message));
            handlers.reserve(std::distance(pair.first, pair.second));
            std::transform(pair.first, pair.second, std::back_inserter(handlers), [](auto entry) {
                return std::static_pointer_cast<message_handler<Message, node_ref>>(entry.second);
            });
        }

        const auto addr = address();

        for(auto &handler : handlers)
            handler->handle_message(msg, addr);
    }

  template <typename Message>
    auto connect(executor ex, std::function<void (Message, node_ref)> f) {
        auto ptr = std::make_shared<deferred_message_handler<Message, node_ref>>(std::move(ex), std::move(f));
        boost::lock_guard<decltype(_lock)> lk(_lock);
        _map.emplace(typeid(Message), std::move(ptr));
    }

  template <typename Message>
    auto connect(node *target, std::function<void (Message, node_ref)> f) {
        return connect(target->_e, std::move(f));
    }

    /// Construct the node to use the specified executor for all message handlers.
    explicit node(executor e = std::experimental::system_executor());

private:
    /// All messages associated with a single actor object should be processed
    /// non-concurrently. We use a strand to ensure non-concurrent execution even
    /// if the underlying executor may use multiple threads.
    strand<executor> _e;

    mutable boost::shared_mutex _lock;

    std::unordered_multimap<std::type_index,
        std::shared_ptr<message_handler_base>> _map;
};

} // namespace ultra

#endif // ULTRA_NODE_H
