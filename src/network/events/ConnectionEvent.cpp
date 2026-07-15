#include "ConnectionEvent.h"

#include <sstream>

namespace network
{

namespace
{
std::string protocolToString(ProtocolType protocol)
{
    switch (protocol)
    {
    case ProtocolType::TCP:
        return "TCP";
    case ProtocolType::UDP:
        return "UDP";
    default:
        return "UNKNOWN";
    }
}

std::string formatEndpoint(const Connection& connection)
{
    // Prefer remote destination when available (active sessions).
    // Fall back to local bind for UDP / listening endpoints.
    if (!connection.remoteAddress.empty())
    {
        std::ostringstream endpoint;
        endpoint << connection.remoteAddress << ":" << connection.remotePort;
        return endpoint.str();
    }

    std::ostringstream endpoint;
    endpoint << connection.localAddress << ":" << connection.localPort;
    return endpoint.str();
}
} // namespace

ConnectionEvent::ConnectionEvent(
    ConnectionEventType type,
    const Connection& connection,
    std::chrono::system_clock::time_point timestamp)
    : type(type),
      connection(connection),
      timestamp(timestamp)
{
}

std::string ConnectionEvent::toString() const
{
    const char* label =
        (type == ConnectionEventType::CREATED) ? "Connection Created"
                                               : "Connection Closed";

    std::ostringstream out;
    out << label << ":\n"
        << "PID " << connection.processId
        << " [" << protocolToString(connection.protocol) << "] -> "
        << formatEndpoint(connection);

    return out.str();
}

} // namespace network
