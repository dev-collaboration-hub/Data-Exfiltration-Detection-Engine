#include "Connection.h"
#include <string>

namespace network
{

Connection::Connection(
    uint32_t processId,
    const std::string& localAddress,
    uint16_t localPort,
    const std::string& remoteAddress,
    uint16_t remotePort,
    ProtocolType protocol,
    ConnectionState state,
    std::chrono::system_clock::time_point timestamp)
    : processId(processId),
      localAddress(localAddress),
      localPort(localPort),
      remoteAddress(remoteAddress),
      remotePort(remotePort),
      protocol(protocol),
      state(state),
      timestamp(timestamp)
{
}

} // namespace network