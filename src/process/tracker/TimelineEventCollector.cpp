#include "TimelineEventCollector.h"

#include <chrono>
#include <sstream>

#include "../../network/utils/NetworkUtils.h"

namespace process
{

std::size_t TimelineEventCollector::collectProcessEvents(
    const std::vector<ProcessEvent>& events)
{
    std::size_t added = 0;

    for (const ProcessEvent& event : events)
    {
        if (event.type != ProcessEventType::CREATED)
        {
            continue;
        }

        const uint32_t pid = event.processInfo.processId;
        const std::string key = makeProcessSeenKey(pid);

        ProcessActivityEvent activity(
            event.timestamp,
            pid,
            event.processInfo.processName,
            ProcessActivityEventType::PROCESS_SEEN,
            "",
            "",
            network::ProtocolType::UNKNOWN,
            0,
            0,
            ProcessActivityEvent::defaultDescription(
                ProcessActivityEventType::PROCESS_SEEN));

        if (tryAdd(activity, key))
        {
            ++added;
        }
    }

    return added;
}

std::size_t TimelineEventCollector::collectActiveProcesses(
    const std::vector<ProcessInfo>& processes)
{
    std::size_t added = 0;
    const auto now = std::chrono::system_clock::now();

    for (const ProcessInfo& info : processes)
    {
        const std::string key = makeProcessSeenKey(info.processId);

        ProcessActivityEvent activity(
            now,
            info.processId,
            info.processName,
            ProcessActivityEventType::PROCESS_SEEN,
            "",
            "",
            network::ProtocolType::UNKNOWN,
            0,
            0,
            ProcessActivityEvent::defaultDescription(
                ProcessActivityEventType::PROCESS_SEEN));

        if (tryAdd(activity, key))
        {
            ++added;
        }
    }

    return added;
}

std::size_t TimelineEventCollector::collectConnectionEvents(
    const std::vector<network::ConnectionEvent>& events,
    const std::unordered_map<uint32_t, ProcessInfo>& processLookup)
{
    std::size_t added = 0;

    for (const network::ConnectionEvent& event : events)
    {
        const ProcessActivityEventType activityType =
            (event.type == network::ConnectionEventType::CREATED)
                ? ProcessActivityEventType::CONNECTION_OPENED
                : ProcessActivityEventType::CONNECTION_CLOSED;

        const network::Connection& connection = event.connection;
        const std::string key = makeConnectionKey(activityType, connection);
        const std::string processName = resolveProcessName(
            connection.processId, "", processLookup);

        ProcessActivityEvent activity(
            event.timestamp,
            connection.processId,
            processName,
            activityType,
            formatEndpoint(connection.localAddress, connection.localPort),
            formatEndpoint(connection.remoteAddress, connection.remotePort),
            connection.protocol,
            0,
            0,
            ProcessActivityEvent::defaultDescription(activityType));

        if (tryAdd(activity, key))
        {
            ++added;
        }
    }

    return added;
}

std::size_t TimelineEventCollector::collectUploadStats(
    const std::vector<network::ConnectionUploadStats>& stats)
{
    std::size_t added = 0;

    for (const network::ConnectionUploadStats& entry : stats)
    {
        if (entry.uploadedBytes == 0)
        {
            continue;
        }

        const std::string key = makeUploadKey(entry);

        ProcessActivityEvent activity(
            entry.lastUpdated,
            entry.processId,
            entry.processName,
            ProcessActivityEventType::UPLOAD_ACTIVITY,
            "",
            formatEndpoint(entry.remoteAddress, entry.remotePort),
            entry.protocol,
            entry.uploadedBytes,
            0,
            ProcessActivityEvent::defaultDescription(
                ProcessActivityEventType::UPLOAD_ACTIVITY));

        if (tryAdd(activity, key))
        {
            ++added;
        }
    }

    return added;
}

std::size_t TimelineEventCollector::collectDownloadStats(
    const std::vector<network::ConnectionDownloadStats>& stats)
{
    std::size_t added = 0;

    for (const network::ConnectionDownloadStats& entry : stats)
    {
        if (entry.downloadedBytes == 0)
        {
            continue;
        }

        const std::string key = makeDownloadKey(entry);

        ProcessActivityEvent activity(
            entry.lastUpdated,
            entry.processId,
            entry.processName,
            ProcessActivityEventType::DOWNLOAD_ACTIVITY,
            "",
            formatEndpoint(entry.remoteAddress, entry.remotePort),
            entry.protocol,
            0,
            entry.downloadedBytes,
            ProcessActivityEvent::defaultDescription(
                ProcessActivityEventType::DOWNLOAD_ACTIVITY));

        if (tryAdd(activity, key))
        {
            ++added;
        }
    }

    return added;
}

std::size_t TimelineEventCollector::collect(
    const std::vector<ProcessEvent>& processEvents,
    const std::vector<network::ConnectionEvent>& connectionEvents,
    const std::vector<network::ConnectionUploadStats>& uploadStats,
    const std::vector<network::ConnectionDownloadStats>& downloadStats,
    const std::unordered_map<uint32_t, ProcessInfo>& processLookup)
{
    std::size_t added = 0;
    added += collectProcessEvents(processEvents);
    added += collectConnectionEvents(connectionEvents, processLookup);
    added += collectUploadStats(uploadStats);
    added += collectDownloadStats(downloadStats);
    return added;
}

const std::unordered_map<uint32_t, std::vector<ProcessActivityEvent>>&
TimelineEventCollector::getEventsByProcess() const
{
    return eventsByProcess_;
}

std::vector<ProcessActivityEvent> TimelineEventCollector::getEventsForProcess(
    uint32_t processId) const
{
    const auto it = eventsByProcess_.find(processId);
    if (it == eventsByProcess_.end())
    {
        return {};
    }
    return it->second;
}

std::vector<ProcessActivityEvent> TimelineEventCollector::getAllEvents() const
{
    std::vector<ProcessActivityEvent> all;
    all.reserve(eventCount());

    for (const auto& [pid, events] : eventsByProcess_)
    {
        (void)pid;
        all.insert(all.end(), events.begin(), events.end());
    }

    return all;
}

std::size_t TimelineEventCollector::eventCount() const
{
    std::size_t total = 0;
    for (const auto& [pid, events] : eventsByProcess_)
    {
        (void)pid;
        total += events.size();
    }
    return total;
}

void TimelineEventCollector::reset()
{
    eventsByProcess_.clear();
    seenEventKeys_.clear();
}

bool TimelineEventCollector::tryAdd(
    const ProcessActivityEvent& event,
    const std::string& dedupeKey)
{
    if (!seenEventKeys_.insert(dedupeKey).second)
    {
        return false;
    }

    eventsByProcess_[event.processId].push_back(event);
    return true;
}

std::string TimelineEventCollector::formatEndpoint(
    const std::string& address,
    uint16_t port)
{
    if (address.empty())
    {
        return {};
    }

    if (network::NetworkUtils::hasRemotePort(port))
    {
        std::ostringstream out;
        out << address << ":" << port;
        return out.str();
    }

    return address;
}

std::string TimelineEventCollector::resolveProcessName(
    uint32_t processId,
    const std::string& preferredName,
    const std::unordered_map<uint32_t, ProcessInfo>& processLookup)
{
    if (!preferredName.empty())
    {
        return preferredName;
    }

    const auto it = processLookup.find(processId);
    if (it != processLookup.end())
    {
        return it->second.processName;
    }

    return {};
}

std::string TimelineEventCollector::makeProcessSeenKey(uint32_t processId)
{
    return "process_seen|" + std::to_string(processId);
}

std::string TimelineEventCollector::makeConnectionKey(
    ProcessActivityEventType type,
    const network::Connection& connection)
{
    std::ostringstream key;
    key << ProcessActivityEvent::eventTypeToString(type) << "|"
        << connection.processId << "|"
        << static_cast<int>(connection.protocol) << "|"
        << connection.localAddress << "|"
        << connection.localPort << "|"
        << connection.remoteAddress << "|"
        << connection.remotePort;
    return key.str();
}

std::string TimelineEventCollector::makeUploadKey(
    const network::ConnectionUploadStats& stats)
{
    std::ostringstream key;
    key << "upload_activity|"
        << stats.processId << "|"
        << static_cast<int>(stats.protocol) << "|"
        << stats.remoteAddress << "|"
        << stats.remotePort << "|"
        << stats.uploadedBytes;
    return key.str();
}

std::string TimelineEventCollector::makeDownloadKey(
    const network::ConnectionDownloadStats& stats)
{
    std::ostringstream key;
    key << "download_activity|"
        << stats.processId << "|"
        << static_cast<int>(stats.protocol) << "|"
        << stats.remoteAddress << "|"
        << stats.remotePort << "|"
        << stats.downloadedBytes;
    return key.str();
}

} // namespace process
