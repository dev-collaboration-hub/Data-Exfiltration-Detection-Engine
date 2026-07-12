#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace network
{

/**
 * @brief Supported network transport protocols.
 */
enum class ProtocolType
{
    TCP,
    UDP,
    UNKNOWN
};

/**
 * @brief Represents the current state of a network connection.
 */
enum class ConnectionState
{
    LISTENING,
    ESTABLISHED,
    TIME_WAIT,
    CLOSE_WAIT,
    CLOSED,
    UNKNOWN
};

/**
 * @brief Represents a single network connection.
 *
 * This model is shared across the Network Monitor,
 * Connection Tracker, Detection Engine, and Reporting modules.
 * It stores connection metadata only and does not contain
 * any operating system or networking logic.
 */
class Connection
{
public:

    /**
     * @brief Default constructor.
     */
    Connection() = default;

    /**
     * @brief Parameterized constructor.
     *
     * @param processId Process ID owning the connection.
     * @param localAddress Local IP address.
     * @param localPort Local port number.
     * @param remoteAddress Remote IP address.
     * @param remotePort Remote port number.
     * @param protocol Transport protocol.
     * @param state Current connection state.
     * @param timestamp Time when the connection was observed.
     */
    Connection(
        uint32_t processId,
        const std::string& localAddress,
        uint16_t localPort,
        const std::string& remoteAddress,
        uint16_t remotePort,
        ProtocolType protocol,
        ConnectionState state,
        std::chrono::system_clock::time_point timestamp);

    /// Process identifier owning this connection.
    uint32_t processId = 0;

    /// Local IP address.
    std::string localAddress;

    /// Local port number.
    uint16_t localPort = 0;

    /// Remote IP address.
    std::string remoteAddress;

    /// Remote port number.
    uint16_t remotePort = 0;

    /// Transport protocol.
    ProtocolType protocol = ProtocolType::UNKNOWN;

    /// Current connection state.
    ConnectionState state = ConnectionState::UNKNOWN;

    /// Time when the connection was observed.
    std::chrono::system_clock::time_point timestamp;
};

} // namespace network
