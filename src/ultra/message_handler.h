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
#ifndef ULTRA_MESSAGE_HANDLER_H
#define ULTRA_MESSAGE_HANDLER_H

#include <experimental/executor>
#include <functional>
#include <typeindex>

namespace ultra {

using std::experimental::executor;
using std::experimental::defer;

/// Base class for all registered message handlers.
class message_handler_base
{
public:
    virtual ~message_handler_base() = default;

    /// Used to determine which message handlers receive an incoming message.
    virtual std::type_index message_id() const = 0;
};

/// Base class for a handler for a specific message and address types.
template <typename Message, typename Address>
class message_handler : public message_handler_base
{
public:
    /// Handle an incoming message.
    virtual void handle_message(Message msg, Address sender) & = 0;

    /// Handle an incoming message.
    virtual void handle_message(Message msg, Address sender) && = 0;
};

/// Message handler for deferred message handling.
template <typename Message, typename Address>
class deferred_message_handler : public message_handler<Message, Address>
{
public:
    /// Construct a message handler to invoke the specified function.
    explicit deferred_message_handler(executor ex,
            std::function<void (Message, Address)> f)
        : _ex(std::move(ex)), _f(std::move(f))
    {
    }

    /// Used to determine which message handlers receive an incoming message.
    virtual std::type_index message_id() const override
    {
        return typeid(Message);
    }

    /// Handle an incoming message.
    virtual void handle_message(Message msg, Address sender) & override
    {
        defer(_ex, std::bind(_f, std::move(msg), std::move(sender)));
    }

    /// Handle an incoming message.
    virtual void handle_message(Message msg, Address sender) && override
    {
        defer(std::move(_ex), std::bind(std::move(_f), std::move(msg), std::move(sender)));
    }

private:
    executor _ex;
    std::function<void (Message, Address)> _f;
};

} // namespace ultra

#endif // ULTRA_MESSAGE_HANDLER_H
