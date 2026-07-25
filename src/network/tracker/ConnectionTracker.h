#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../events/ConnectionEvent.h"
#include "../models/Connection.h"

namespace network
{

/**
 * @brief Tracks active connections across snapshots and emits lifecycle events.
 *
 * Stores the previous connection set, compares it with each new snapshot from
 * NetworkMonitor, and generates CREATED / CLOSED events for changes.
 */
class ConnectionTracker
{
public:
    /**
     * @brief Compare a new connection snapshot against the previous state.
     *
     * @param currentConnections Latest connections from NetworkMonitor.
     * @return Events for newly created and closed connections.
     */
    std::vector<ConnectionEvent> update(const std::vector<Connection>& currentConnections);

    /// Currently known active connections keyed by connection identity.
    const std::unordered_map<std::string, Connection>& getActiveConnections() const;

    /// Clears stored connection state (next update will re-seed silently).
    void reset();

private:
    static std::string makeKey(const Connection& connection);

    std::unordered_map<std::string, Connection> activeConnections_;
    bool seeded_ = false;
};

} // namespace network
