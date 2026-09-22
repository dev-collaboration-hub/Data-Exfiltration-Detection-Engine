#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../events/ProcessActivityEvent.h"
#include "../events/ProcessEvent.h"
#include "../models/ProcessInfo.h"
#include "../../network/events/ConnectionEvent.h"
#include "../../network/models/ConnectionDownloadStats.h"
#include "../../network/models/ConnectionUploadStats.h"

namespace process
{

/**
 * @brief Collects process, connection, and transfer activity into a timeline.
 *
 * Consumes outputs from ProcessTracker, ConnectionTracker, UploadTracker, and
 * DownloadTracker, converts them into reusable ProcessActivityEvent records,
 * groups them by PID, and skips duplicate observations (Milestone M5).
 */
class TimelineEventCollector
{
public:
    /**
     * @brief Collect PROCESS_SEEN events from process lifecycle changes.
     *
     * Only CREATED events are recorded. Empty input is a no-op.
     *
     * @return Number of newly stored timeline events.
     */
    std::size_t collectProcessEvents(const std::vector<ProcessEvent>& events);

    /**
     * @brief Collect PROCESS_SEEN events for currently active processes.
     *
     * Useful when seeding a timeline from a ProcessMonitor snapshot.
     * Duplicate PIDs within the snapshot and across collections are skipped.
     *
     * @return Number of newly stored timeline events.
     */
    std::size_t collectActiveProcesses(const std::vector<ProcessInfo>& processes);

    /**
     * @brief Collect CONNECTION_OPENED / CONNECTION_CLOSED timeline events.
     *
     * Network events are attributed to connection.processId. Optional
     * @p processLookup supplies process names when not present elsewhere.
     *
     * @return Number of newly stored timeline events.
     */
    std::size_t collectConnectionEvents(
        const std::vector<network::ConnectionEvent>& events,
        const std::unordered_map<uint32_t, ProcessInfo>& processLookup = {});

    /**
     * @brief Collect UPLOAD_ACTIVITY events from upload tracker statistics.
     *
     * Emits an event when uploaded bytes are greater than zero. Repeated
     * collection with the same totals is ignored.
     *
     * @return Number of newly stored timeline events.
     */
    std::size_t collectUploadStats(
        const std::vector<network::ConnectionUploadStats>& stats);

    /**
     * @brief Collect DOWNLOAD_ACTIVITY events from download tracker statistics.
     *
     * Emits an event when downloaded bytes are greater than zero. Repeated
     * collection with the same totals is ignored.
     *
     * @return Number of newly stored timeline events.
     */
    std::size_t collectDownloadStats(
        const std::vector<network::ConnectionDownloadStats>& stats);

    /**
     * @brief Collect from all sources in one call.
     *
     * Empty vectors are safe and produce no events.
     *
     * @return Total number of newly stored timeline events.
     */
    std::size_t collect(
        const std::vector<ProcessEvent>& processEvents,
        const std::vector<network::ConnectionEvent>& connectionEvents,
        const std::vector<network::ConnectionUploadStats>& uploadStats,
        const std::vector<network::ConnectionDownloadStats>& downloadStats,
        const std::unordered_map<uint32_t, ProcessInfo>& processLookup = {});

    /// Timeline events grouped by owning process ID.
    const std::unordered_map<uint32_t, std::vector<ProcessActivityEvent>>&
    getEventsByProcess() const;

    /// Events collected for a single PID (empty when unknown).
    std::vector<ProcessActivityEvent> getEventsForProcess(uint32_t processId) const;

    /// Flat list of all collected events (grouped-PID iteration order).
    std::vector<ProcessActivityEvent> getAllEvents() const;

    /// Total number of stored timeline events across all processes.
    std::size_t eventCount() const;

    /// Clears collected events and deduplication state.
    void reset();

private:
    bool tryAdd(const ProcessActivityEvent& event, const std::string& dedupeKey);

    static std::string formatEndpoint(const std::string& address, uint16_t port);

    static std::string resolveProcessName(
        uint32_t processId,
        const std::string& preferredName,
        const std::unordered_map<uint32_t, ProcessInfo>& processLookup);

    static std::string makeProcessSeenKey(uint32_t processId);

    static std::string makeConnectionKey(
        ProcessActivityEventType type,
        const network::Connection& connection);

    static std::string makeUploadKey(const network::ConnectionUploadStats& stats);

    static std::string makeDownloadKey(const network::ConnectionDownloadStats& stats);

    std::unordered_map<uint32_t, std::vector<ProcessActivityEvent>> eventsByProcess_;
    std::unordered_set<std::string> seenEventKeys_;
};

} // namespace process
