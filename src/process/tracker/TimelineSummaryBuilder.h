#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../events/ProcessActivityEvent.h"
#include "../models/ProcessActivitySummary.h"
#include "ProcessTimelineBuilder.h"
#include "TimelineEventHistory.h"

namespace process
{

/**
 * @brief Converts raw timeline events into human-readable activity summaries.
 *
 * Produces process activity, upload/download, connection, and soft behavior
 * hint context for Milestone M5. Does not emit final security alerts (M6).
 */
class TimelineSummaryBuilder
{
public:
    /// Soft hint when outbound bytes exceed this threshold (10 MiB).
    static constexpr uint64_t kElevatedUploadBytes = 10ull * 1024ull * 1024ull;

    /// Soft hint when outbound is at least this multiple of inbound.
    static constexpr uint64_t kUploadDominantRatio = 5;

    /// Soft hint when many distinct remotes are observed.
    static constexpr std::size_t kManyRemoteEndpoints = 8;

    /// Soft hint when repeated upload observations are seen.
    static constexpr std::size_t kRepeatedUploadEvents = 5;

    TimelineSummaryBuilder() = default;

    /// Build summaries from an explicit event list.
    explicit TimelineSummaryBuilder(std::vector<ProcessActivityEvent> events);

    /// Build summaries from short-term history.
    explicit TimelineSummaryBuilder(const TimelineEventHistory& history);

    /// Build summaries from a sorted global timeline.
    explicit TimelineSummaryBuilder(const ProcessTimelineBuilder& builder);

    /// Replace the event source and clear cached summaries.
    void setEvents(std::vector<ProcessActivityEvent> events);

    /// Summary for a specific PID (empty summary when unknown).
    ProcessActivitySummary forProcessId(uint32_t processId) const;

    /// Summary for an exact process name (aggregates matching events).
    ProcessActivitySummary forProcessName(const std::string& processName) const;

    /// One summary per distinct PID, ordered by PID ascending.
    std::vector<ProcessActivitySummary> forAllProcesses() const;

    /// Number of source events available to summarize.
    std::size_t eventCount() const;

private:
    static ProcessActivitySummary summarize(
        const std::vector<ProcessActivityEvent>& events);

    static std::vector<std::string> buildBehaviorHints(
        const ProcessActivitySummary& summary,
        std::size_t uploadEventCount,
        std::size_t suspiciousEventCount,
        const std::vector<std::string>& suspiciousDescriptions);

    std::vector<ProcessActivityEvent> events_;
};

} // namespace process
