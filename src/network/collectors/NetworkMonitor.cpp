#include "NetworkMonitor.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace network
{

namespace
{
static ConnectionState toConnectionState(DWORD windowsState)
{
    switch (windowsState)
    {
    case MIB_TCP_STATE_LISTEN:
        return ConnectionState::LISTENING;
    case MIB_TCP_STATE_ESTAB:
        return ConnectionState::ESTABLISHED;
    case MIB_TCP_STATE_TIME_WAIT:
        return ConnectionState::TIME_WAIT;
    case MIB_TCP_STATE_CLOSE_WAIT:
        return ConnectionState::CLOSE_WAIT;
    case MIB_TCP_STATE_CLOSED:
        return ConnectionState::CLOSED;
    default:
        return ConnectionState::UNKNOWN;
    }
}

static std::string ipv4ToString(DWORD addr)
{
    std::array<char, INET_ADDRSTRLEN> buf{};
    in_addr a{};
    a.S_un.S_addr = addr;
    if (inet_ntop(AF_INET, &a, buf.data(), static_cast<socklen_t>(buf.size())) == nullptr)
    {
        return {};
    }
    return std::string(buf.data());
}

static void collectTcpConnections(std::vector<Connection>& connections,
                                  std::chrono::system_clock::time_point now)
{
    ULONG size = 0;
    DWORD ret = GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (ret != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        return;
    }

    std::vector<std::uint8_t> buffer(size);
    ret = GetExtendedTcpTable(buffer.data(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (ret != NO_ERROR)
    {
        return;
    }

    const auto* table = reinterpret_cast<const MIB_TCPTABLE_OWNER_PID*>(buffer.data());
    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        const MIB_TCPROW_OWNER_PID& row = table->table[i];

        connections.emplace_back(
            row.dwOwningPid,
            ipv4ToString(row.dwLocalAddr),
            ntohs(static_cast<u_short>(row.dwLocalPort)),
            ipv4ToString(row.dwRemoteAddr),
            ntohs(static_cast<u_short>(row.dwRemotePort)),
            ProtocolType::TCP,
            toConnectionState(row.dwState),
            now);
    }
}

// UDP is connectionless: IP Helper exposes local endpoints only
// (no remote address/port and no TCP-style connection state).
static void collectUdpEndpoints(std::vector<Connection>& connections,
                                std::chrono::system_clock::time_point now)
{
    ULONG size = 0;
    DWORD ret = GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (ret != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        return;
    }

    std::vector<std::uint8_t> buffer(size);
    ret = GetExtendedUdpTable(buffer.data(), &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (ret != NO_ERROR)
    {
        return;
    }

    const auto* table = reinterpret_cast<const MIB_UDPTABLE_OWNER_PID*>(buffer.data());
    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        const MIB_UDPROW_OWNER_PID& row = table->table[i];

        connections.emplace_back(
            row.dwOwningPid,
            ipv4ToString(row.dwLocalAddr),
            ntohs(static_cast<u_short>(row.dwLocalPort)),
            std::string{},
            static_cast<uint16_t>(0),
            ProtocolType::UDP,
            ConnectionState::LISTENING,
            now);
    }
}
} // namespace

std::vector<Connection> NetworkMonitor::getActiveConnections()
{
    std::vector<Connection> connections;
    const auto now = std::chrono::system_clock::now();

    collectTcpConnections(connections, now);
    collectUdpEndpoints(connections, now);

    return connections;
}

} // namespace network
