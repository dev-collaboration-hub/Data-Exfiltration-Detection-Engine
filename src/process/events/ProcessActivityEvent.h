#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "../../network/models/Connection.h"

namespace process
{

/**
 * @brief Activity kinds stored on a process behavior timeline (Milestone M5).
 *
 * Labels match the snake_case names used in reporting and sample output.
 */
enum class ProcessActivityEventType
{
    PROCESS_SEEN,
    CONNECTION_OPENED,
    UPLOAD_ACTIVITY,
    DOWNLOAD_ACTIVITY,
    CONNECTION_CLOSED,
    SUSPICIOUS_ACTIVITY
};

/**
 * @brief Reusable timeline event for process network activity.
 *
 * Stores a single dated observation that later timeline builders and
 * detection modules can order, filter, and print. Holds event data only —
 * no OS monitoring or detection logic.
 */
class ProcessActivityEvent
{
public:
    ProcessActivityEvent() = default;

    ProcessActivityEvent(
        std::chrono::system_clock::time_point timestamp,
        uint32_t processId,
        const std::string& processName,
        ProcessActivityEventType eventType,
        const std::string& localAddress,
        const std::string& remoteAddress,
        network::ProtocolType protocol,
        uint64_t uploadedBytes,
        uint64_t downloadedBytes,
        const std::string& description);

    /// When the activity was observed.
    std::chrono::system_clock::time_point timestamp;

    /// Process ID associated with the activity.
    uint32_t processId = 0;

    /// Executable name when known (may be empty).
    std::string processName;

    /// Timeline event kind.
    ProcessActivityEventType eventType = ProcessActivityEventType::PROCESS_SEEN;

    /// Local endpoint address (IP or IP:port; may be empty).
    std::string localAddress;

    /// Remote endpoint address (IP or IP:port; may be empty).
    std::string remoteAddress;

    /// Transport protocol when known.
    network::ProtocolType protocol = network::ProtocolType::UNKNOWN;

    /// Cumulative outbound bytes associated with this event (if any).
    uint64_t uploadedBytes = 0;

    /// Cumulative inbound bytes associated with this event (if any).
    uint64_t downloadedBytes = 0;

    /// Human-readable summary for timeline / report display.
    std::string description;

    /// Snake_case label for @p type (e.g. "process_seen").
    static std::string eventTypeToString(ProcessActivityEventType type);

    /// Default timeline phrase for @p type (e.g. "Process Started").
    static std::string defaultDescription(ProcessActivityEventType type);

    /// Formats the observation time as UTC HH:MM for timeline display.
    std::string formatTimestamp() const;

    /**
     * @brief Compact timeline line for chronological listings.
     *
     * Example:
     *   10:01 Process Started
     */
    std::string toTimelineString() const;

    /**
     * @brief Formats the full event for logging / inspection.
     *
     * Example:
     *   Timestamp:
     *   10:01
     *
     *   PID:
     *   4120
     *
     *   Process:
     *   python.exe
     *
     *   Event Type:
     *   process_seen
     *
     *   Local Address:
     *   192.168.1.10:53142
     *
     *   Remote Address:
     *   104.18.32.45:443
     *
     *   Protocol:
     *   TCP
     *
     *   Uploaded:
     *   0 B
     *
     *   Downloaded:
     *   0 B
     *
     *   Description:
     *   Process Started
     */
    std::string toString() const;
};

} // namespace process
