#include "ConnectionTracker.h"

#include <chrono>
#include <sstream>
#include <unordered_set>

namespace network
{

std::string ConnectionTracker::makeKey(const Connection& connection)
{
    // Identity key: protocol + PID + 4-tuple.
    // This avoids treating the same endpoint under different PIDs as one connection.
    std::ostringstream key;
    key << static_cast<int>(connection.protocol) << "|"
        << connection.processId << "|"
        << connection.localAddress << "|"
        << connection.localPort << "|"
        << connection.remoteAddress << "|"
        << connection.remotePort;
    return key.str();
}

std::vector<ConnectionEvent> ConnectionTracker::update(
    const std::vector<Connection>& currentConnections)
{
    std::vector<ConnectionEvent> events;
    const auto now = std::chrono::system_clock::now();

    std::unordered_map<std::string, Connection> nextActive;
    nextActive.reserve(currentConnections.size());

    std::unordered_set<std::string> seen;
    seen.reserve(currentConnections.size());

    for (const Connection& connection : currentConnections)
    {
        const std::string key = makeKey(connection);
        if (!seen.insert(key).second)
        {
            continue; // Skip duplicate rows within the same snapshot.
        }

        nextActive.emplace(key, connection);
    }

    // First snapshot only establishes baseline state (no create spam).
    if (!seeded_)
    {
        activeConnections_ = std::move(nextActive);
        seeded_ = true;
        return events;
    }

    // Detect newly created connections.
    for (const auto& [key, connection] : nextActive)
    {
        if (activeConnections_.find(key) == activeConnections_.end())
        {
            events.emplace_back(ConnectionEventType::CREATED, connection, now);
        }
    }

    // Detect closed connections (present previously, missing now).
    for (const auto& [key, previous] : activeConnections_)
    {
        if (nextActive.find(key) == nextActive.end())
        {
            events.emplace_back(ConnectionEventType::CLOSED, previous, now);
        }
    }

    activeConnections_ = std::move(nextActive);
    return events;
}

const std::unordered_map<std::string, Connection>&
ConnectionTracker::getActiveConnections() const
{
    return activeConnections_;
}

void ConnectionTracker::reset()
{
    activeConnections_.clear();
    seeded_ = false;
}

} // namespace network
