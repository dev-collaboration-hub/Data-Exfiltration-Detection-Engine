#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "Connection.h"
#include "ConnectionBandwidthStats.h"

namespace network
{

/**
 * @brief Reusable metadata for network data-transfer activity and bandwidth.
 *
 * Combines process attribution, remote endpoint, protocol, cumulative
 * upload/download volumes, transfer rates, and observation timestamps into
 * a single portable record for Data Transfer Analysis (Milestone M4).
 * Stores metadata only — no OS networking logic.
 */
class DataTransferMetadata
{
public:
    DataTransferMetadata() = default;

    DataTransferMetadata(
        uint32_t processId,
        const std::string& processName,
        const std::string& remoteIp,
        uint16_t remotePort,
        ProtocolType protocol,
        uint64_t uploadedBytes,
        uint64_t downloadedBytes,
        double uploadRateBps,
        double downloadRateBps,
        std::chrono::system_clock::time_point firstObserved,
        std::chrono::system_clock::time_point lastUpdated);

    /**
     * @brief Build transfer metadata from a Connection and measured metrics.
     *
     * @param connection Source connection (IP, port, protocol, PID).
     * @param uploadedBytes Cumulative outbound bytes.
     * @param downloadedBytes Cumulative inbound bytes.
     * @param uploadRateBps Short-window upload rate (bytes/second).
     * @param downloadRateBps Short-window download rate (bytes/second).
     * @param processName Executable name when known (may be empty).
     * @param firstObserved When transfer tracking started (defaults to
     *        connection.timestamp when unset).
     */
    static DataTransferMetadata fromConnection(
        const Connection& connection,
        uint64_t uploadedBytes,
        uint64_t downloadedBytes,
        double uploadRateBps = 0.0,
        double downloadRateBps = 0.0,
        const std::string& processName = "",
        std::chrono::system_clock::time_point firstObserved = {});

    /**
     * @brief Build transfer metadata from existing bandwidth statistics.
     */
    static DataTransferMetadata fromBandwidthStats(
        const ConnectionBandwidthStats& stats);

    /// Process ID that owns the connection.
    uint32_t processId = 0;

    /// Executable name when known (may be empty).
    std::string processName;

    /// Remote peer IP address.
    std::string remoteIp;

    /// Remote peer port (0 when unavailable).
    uint16_t remotePort = 0;

    /// Transport protocol (TCP / UDP / UNKNOWN).
    ProtocolType protocol = ProtocolType::UNKNOWN;

    /// Cumulative outbound bytes transferred.
    uint64_t uploadedBytes = 0;

    /// Cumulative inbound bytes transferred.
    uint64_t downloadedBytes = 0;

    /// Short-window upload rate in bytes per second.
    double uploadRateBps = 0.0;

    /// Short-window download rate in bytes per second.
    double downloadRateBps = 0.0;

    /// When transfer tracking started for this observation.
    std::chrono::system_clock::time_point firstObserved;

    /// When transfer counters were last updated.
    std::chrono::system_clock::time_point lastUpdated;

    /// Total bytes transferred (upload + download).
    uint64_t totalTransferredBytes() const;

    /**
     * @brief Formats data-transfer metadata for logging / inspection.
     *
     * Example:
     *   Process:
     *   python.exe
     *
     *   Protocol:
     *   TCP
     *
     *   Remote:
     *   104.18.32.45:443
     *
     *   Uploaded:
     *   12.5 MB
     *
     *   Downloaded:
     *   3.2 MB
     *
     *   Upload Rate:
     *   1.4 MB/s
     *
     *   Download Rate:
     *   320.0 KB/s
     *
     *   Total Transfer:
     *   15.7 MB
     */
    std::string toString() const;
};

} // namespace network
