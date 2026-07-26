#pragma once

#include <chrono>
#include <string>

#include "../models/Connection.h"

namespace network
{

enum class ConnectionEventType
{
    CREATED,
    CLOSED
};

/**
 * @brief Represents a connection lifecycle change detected by the tracker.
 */
class ConnectionEvent
{
public:
    ConnectionEvent() = default;

    ConnectionEvent(
        ConnectionEventType type,
        const Connection& connection,
        std::chrono::system_clock::time_point timestamp);

    ConnectionEventType type = ConnectionEventType::CREATED;
    Connection connection;
    std::chrono::system_clock::time_point timestamp;

    /**
     * @brief Formats the event for logging / inspection.
     *
     * Examples:
     *   Connection Created: PID 4020 -> 142.250.0.1:443
     *   Connection Closed:  PID 1280 -> 93.184.216.34:443
     */
    std::string toString() const;
};

} // namespace network
