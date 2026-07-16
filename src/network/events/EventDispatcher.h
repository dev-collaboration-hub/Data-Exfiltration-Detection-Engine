#pragma once

#include <functional>
#include <vector>

#include "ConnectionEvent.h"

namespace network
{

/**
 * @brief Publishes connection lifecycle events to subscribed consumers.
 *
 * Downstream modules (Correlation Engine, Detection Engine, logging)
 * can subscribe without depending on NetworkMonitor or ConnectionTracker.
 */
class EventDispatcher
{
public:
    using EventHandler = std::function<void(const ConnectionEvent&)>;

    /// Register a handler that will receive future connection events.
    void subscribe(EventHandler handler);

    /// Publish a batch of connection events to all subscribers.
    void dispatch(const std::vector<ConnectionEvent>& events);

    /// Publish a single connection event to all subscribers.
    void dispatch(const ConnectionEvent& event);

    /// Remove all registered handlers.
    void clear();

private:
    std::vector<EventHandler> handlers_;
};

} // namespace network
