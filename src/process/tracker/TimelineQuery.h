#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../events/ProcessActivityEvent.h"
#include "ProcessTimelineBuilder.h"
#include "TimelineEventHistory.h"

namespace process
{

/**
 * @brief Simple query helpers for timeline / history event data.
 *
 * Filters ProcessActivityEvent collections by process identity, remote
 * destination, event type, or recency (Milestone M5).
 */
class TimelineQuery
{
public:
    /// Query against an explicit event list (copied).
    explicit TimelineQuery(std::vector<ProcessActivityEvent> events);

    /// Query against events currently retained in short-term history.
    explicit TimelineQuery(const TimelineEventHistory& history);

    /// Query against a built global timeline.
    explicit TimelineQuery(const ProcessTimelineBuilder& builder);

    /// Chronological events for a specific PID.
    std::vector<ProcessActivityEvent> byProcessId(uint32_t processId) const;

    /// Chronological events for an exact process name.
    std::vector<ProcessActivityEvent> byProcessName(
        const std::string& processName) const;

    /**
     * @brief Chronological events for a remote IP or domain.
     *
     * Matches exact remote endpoints and host-only queries against
     * `host:port` values (e.g. "104.18.32.45" matches "104.18.32.45:443").
     * Also matches when the query appears in the event description.
     */
    std::vector<ProcessActivityEvent> byRemoteDestination(
        const std::string& remoteIpOrDomain) const;

    /// Chronological events of a specific activity type.
    std::vector<ProcessActivityEvent> byEventType(
        ProcessActivityEventType eventType) const;

    /**
     * @brief Newest @p count events in chronological order.
     *
     * When @p count is 0, all events are returned.
     */
    std::vector<ProcessActivityEvent> recent(std::size_t count) const;

    /// Number of events available to query.
    std::size_t size() const;

private:
    static bool earlierThan(
        const ProcessActivityEvent& left,
        const ProcessActivityEvent& right);

    static bool matchesRemoteDestination(
        const ProcessActivityEvent& event,
        const std::string& remoteIpOrDomain);

    static std::string hostFromEndpoint(const std::string& endpoint);

    static std::vector<ProcessActivityEvent> sortedCopy(
        std::vector<ProcessActivityEvent> events);

    std::vector<ProcessActivityEvent> events_;
};

} // namespace process
