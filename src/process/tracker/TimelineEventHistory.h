#pragma once

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "../events/ProcessActivityEvent.h"
#include "TimelineEventCollector.h"

namespace process
{

/**
 * @brief Short-term in-memory history of recent timeline events.
 *
 * Stores ProcessActivityEvent records with a configurable maximum size and
 * optional maximum age so memory cannot grow without bound. Supports lookup
 * by process ID or process name (Milestone M5).
 */
class TimelineEventHistory
{
public:
    /// Default maximum number of retained events.
    static constexpr std::size_t kDefaultMaxEvents = 1000;

    /**
     * @param maxEvents Maximum events retained (minimum 1).
     * @param maxAge Maximum age of retained events; zero disables age expiry.
     */
    explicit TimelineEventHistory(
        std::size_t maxEvents = kDefaultMaxEvents,
        std::chrono::seconds maxAge = std::chrono::seconds(3600));

    /// Append one event and enforce retention limits.
    void add(const ProcessActivityEvent& event);

    /// Append many events and enforce retention limits once.
    void add(const std::vector<ProcessActivityEvent>& events);

    /// Append all events currently held by a collector.
    void add(const TimelineEventCollector& collector);

    /// Chronological events for a specific PID (empty when none).
    std::vector<ProcessActivityEvent> getByProcessId(uint32_t processId) const;

    /// Chronological events matching process name (exact match; empty when none).
    std::vector<ProcessActivityEvent> getByProcessName(
        const std::string& processName) const;

    /// Recent events in chronological order (optionally capped by @p limit).
    std::vector<ProcessActivityEvent> getRecent(std::size_t limit = 0) const;

    /// Number of events currently retained.
    std::size_t size() const;

    /// Configured maximum event count.
    std::size_t maxEvents() const;

    /// Update the maximum event count and enforce it immediately.
    void setMaxEvents(std::size_t maxEvents);

    /// Configured maximum event age (zero means age cleanup is disabled).
    std::chrono::seconds maxAge() const;

    /// Update maximum age and enforce retention immediately.
    void setMaxAge(std::chrono::seconds maxAge);

    /**
     * @brief Drop expired / overflow events.
     *
     * Removes events older than maxAge (when configured), then removes the
     * oldest events until size fits within maxEvents.
     *
     * @return Number of events removed.
     */
    std::size_t cleanup();

    /// Remove every stored event.
    void clear();

private:
    static bool earlierThan(
        const ProcessActivityEvent& left,
        const ProcessActivityEvent& right);

    std::size_t maxEvents_;
    std::chrono::seconds maxAge_;
    std::vector<ProcessActivityEvent> events_;
};

} // namespace process
