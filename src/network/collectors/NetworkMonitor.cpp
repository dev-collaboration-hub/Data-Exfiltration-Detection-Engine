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
} // namespace

std::vector<Connection> NetworkMonitor::getActiveConnections()
{
    std::vector<Connection> connections;
    const auto now = std::chrono::system_clock::now();

    ULONG size = 0;
    DWORD ret = GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (ret != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        return connections;
    }

    std::vector<std::uint8_t> buffer(size);
    ret = GetExtendedTcpTable(buffer.data(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (ret != NO_ERROR)
    {
        return connections;
    }

    const auto* table = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
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

    return connections;
}

} // namespace network
