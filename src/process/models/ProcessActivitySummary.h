#pragma once

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace process
{

/**
 * @brief Human-readable process activity summary derived from timeline events.
 *
 * Aggregates duration, remote connections, upload/download totals, the most
 * active destination, and soft behavior hints for Milestone M5 historical
 * analysis. Holds summary data only — no detection alerts or OS logic.
 */
class ProcessActivitySummary
{
public:
    ProcessActivitySummary() = default;

    ProcessActivitySummary(
        uint32_t processId,
        const std::string& processName,
        std::chrono::system_clock::time_point firstSeen,
        std::chrono::system_clock::time_point lastSeen,
        std::size_t remoteEndpointCount,
        std::size_t connectionOpenCount,
        uint64_t totalUploadedBytes,
        uint64_t totalDownloadedBytes,
        const std::string& mostActiveDestination,
        const std::vector<std::string>& behaviorHints);

    /// Process ID summarized (0 when unknown / empty).
    uint32_t processId = 0;

    /// Executable name when known (may be empty).
    std::string processName;

    /// Earliest event timestamp in the summarized window.
    std::chrono::system_clock::time_point firstSeen;

    /// Latest event timestamp in the summarized window.
    std::chrono::system_clock::time_point lastSeen;

    /// Distinct remote endpoints observed.
    std::size_t remoteEndpointCount = 0;

    /// Number of connection_opened observations.
    std::size_t connectionOpenCount = 0;

    /// Peak outbound bytes summed across remote endpoints.
    uint64_t totalUploadedBytes = 0;

    /// Peak inbound bytes summed across remote endpoints.
    uint64_t totalDownloadedBytes = 0;

    /// Remote endpoint with the highest observed transfer volume.
    std::string mostActiveDestination;

    /**
     * @brief Soft behavior hints for later M6 detection context.
     *
     * Hints are descriptive only — not final security alerts.
     */
    std::vector<std::string> behaviorHints;

    /// Active window duration (lastSeen - firstSeen), zero when unset.
    std::chrono::seconds activeDuration() const;

    /// True when no process activity was summarized.
    bool empty() const;

    /**
     * @brief Simple multi-line summary for operators / reports.
     *
     * Example:
     *   python.exe was active for 12 minutes.
     *   It connected to 3 remote endpoints.
     *   Total upload: 25.4 MB.
     *   Total download: 1.2 MB.
     *   Most active destination: api.example.com:443.
     *   Hint: Elevated outbound transfer relative to inbound traffic.
     */
    std::string toSummaryString() const;

    /**
     * @brief Detailed field dump for logging / inspection.
     */
    std::string toString() const;

    /// Formats a duration as a short phrase (e.g. "12 minutes").
    static std::string formatDuration(std::chrono::seconds duration);
};

} // namespace process
