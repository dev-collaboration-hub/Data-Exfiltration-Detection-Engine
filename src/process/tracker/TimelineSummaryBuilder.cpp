#include "TimelineSummaryBuilder.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

namespace process
{

namespace
{

struct DestinationStats
{
    uint64_t uploadedBytes = 0;
    uint64_t downloadedBytes = 0;
    std::size_t eventCount = 0;
};

bool isMoreActive(const DestinationStats& left, const DestinationStats& right)
{
    const uint64_t leftTotal = left.uploadedBytes + left.downloadedBytes;
    const uint64_t rightTotal = right.uploadedBytes + right.downloadedBytes;
    if (leftTotal != rightTotal)
    {
        return leftTotal > rightTotal;
    }
    if (left.uploadedBytes != right.uploadedBytes)
    {
        return left.uploadedBytes > right.uploadedBytes;
    }
    return left.eventCount > right.eventCount;
}

} // namespace

TimelineSummaryBuilder::TimelineSummaryBuilder(
    std::vector<ProcessActivityEvent> events)
    : events_(std::move(events))
{
}

TimelineSummaryBuilder::TimelineSummaryBuilder(
    const TimelineEventHistory& history)
    : events_(history.getRecent())
{
}

TimelineSummaryBuilder::TimelineSummaryBuilder(
    const ProcessTimelineBuilder& builder)
    : events_(builder.getGlobalTimeline())
{
}

void TimelineSummaryBuilder::setEvents(std::vector<ProcessActivityEvent> events)
{
    events_ = std::move(events);
}

ProcessActivitySummary TimelineSummaryBuilder::forProcessId(
    uint32_t processId) const
{
    std::vector<ProcessActivityEvent> matched;
    for (const ProcessActivityEvent& event : events_)
    {
        if (event.processId == processId)
        {
            matched.push_back(event);
        }
    }
    return summarize(matched);
}

ProcessActivitySummary TimelineSummaryBuilder::forProcessName(
    const std::string& processName) const
{
    std::vector<ProcessActivityEvent> matched;
    for (const ProcessActivityEvent& event : events_)
    {
        if (event.processName == processName)
        {
            matched.push_back(event);
        }
    }
    return summarize(matched);
}

std::vector<ProcessActivitySummary> TimelineSummaryBuilder::forAllProcesses()
    const
{
    std::map<uint32_t, std::vector<ProcessActivityEvent>> byPid;
    for (const ProcessActivityEvent& event : events_)
    {
        byPid[event.processId].push_back(event);
    }

    std::vector<ProcessActivitySummary> summaries;
    summaries.reserve(byPid.size());
    for (const auto& [pid, processEvents] : byPid)
    {
        (void)pid;
        summaries.push_back(summarize(processEvents));
    }
    return summaries;
}

std::size_t TimelineSummaryBuilder::eventCount() const
{
    return events_.size();
}

ProcessActivitySummary TimelineSummaryBuilder::summarize(
    const std::vector<ProcessActivityEvent>& events)
{
    if (events.empty())
    {
        return {};
    }

    uint32_t processId = events.front().processId;
    std::string processName = events.front().processName;
    auto firstSeen = events.front().timestamp;
    auto lastSeen = events.front().timestamp;

    std::set<std::string> remotes;
    std::size_t connectionOpenCount = 0;
    std::size_t uploadEventCount = 0;
    std::size_t suspiciousEventCount = 0;
    std::vector<std::string> suspiciousDescriptions;
    std::unordered_map<std::string, DestinationStats> byDestination;

    for (const ProcessActivityEvent& event : events)
    {
        processId = event.processId;
        if (processName.empty() && !event.processName.empty())
        {
            processName = event.processName;
        }

        if (event.timestamp < firstSeen)
        {
            firstSeen = event.timestamp;
        }
        if (event.timestamp > lastSeen)
        {
            lastSeen = event.timestamp;
        }

        if (event.eventType == ProcessActivityEventType::CONNECTION_OPENED)
        {
            ++connectionOpenCount;
        }

        if (event.eventType == ProcessActivityEventType::UPLOAD_ACTIVITY)
        {
            ++uploadEventCount;
        }

        if (event.eventType == ProcessActivityEventType::SUSPICIOUS_ACTIVITY)
        {
            ++suspiciousEventCount;
            if (!event.description.empty())
            {
                suspiciousDescriptions.push_back(event.description);
            }
        }

        if (!event.remoteAddress.empty())
        {
            remotes.insert(event.remoteAddress);
            DestinationStats& stats = byDestination[event.remoteAddress];
            ++stats.eventCount;

            // Growing cumulative counters produce multiple events per remote;
            // keep the peak observed volume for each destination.
            if (event.uploadedBytes > stats.uploadedBytes)
            {
                stats.uploadedBytes = event.uploadedBytes;
            }
            if (event.downloadedBytes > stats.downloadedBytes)
            {
                stats.downloadedBytes = event.downloadedBytes;
            }
        }
    }

    uint64_t totalUploaded = 0;
    uint64_t totalDownloaded = 0;
    std::string mostActiveDestination;
    DestinationStats bestStats;

    for (const auto& [destination, stats] : byDestination)
    {
        totalUploaded += stats.uploadedBytes;
        totalDownloaded += stats.downloadedBytes;

        if (mostActiveDestination.empty() || isMoreActive(stats, bestStats))
        {
            mostActiveDestination = destination;
            bestStats = stats;
        }
    }

    ProcessActivitySummary summary(
        processId,
        processName,
        firstSeen,
        lastSeen,
        remotes.size(),
        connectionOpenCount,
        totalUploaded,
        totalDownloaded,
        mostActiveDestination,
        {});

    summary.behaviorHints = buildBehaviorHints(
        summary,
        uploadEventCount,
        suspiciousEventCount,
        suspiciousDescriptions);

    return summary;
}

std::vector<std::string> TimelineSummaryBuilder::buildBehaviorHints(
    const ProcessActivitySummary& summary,
    std::size_t uploadEventCount,
    std::size_t suspiciousEventCount,
    const std::vector<std::string>& suspiciousDescriptions)
{
    std::vector<std::string> hints;

    if (summary.totalUploadedBytes >= kElevatedUploadBytes)
    {
        hints.push_back("Elevated outbound transfer volume observed.");
    }

    if (summary.totalUploadedBytes > 0 &&
        summary.totalUploadedBytes >=
            summary.totalDownloadedBytes * kUploadDominantRatio &&
        summary.totalUploadedBytes >= (1024ull * 1024ull))
    {
        hints.push_back(
            "Outbound transfer is high relative to inbound traffic.");
    }

    if (summary.remoteEndpointCount >= kManyRemoteEndpoints)
    {
        hints.push_back("Connected to many distinct remote endpoints.");
    }

    if (uploadEventCount >= kRepeatedUploadEvents)
    {
        hints.push_back("Repeated upload activity observed over the timeline.");
    }

    if (suspiciousEventCount > 0)
    {
        if (!suspiciousDescriptions.empty())
        {
            // Keep a single concise hint; avoid turning M5 into alert output.
            hints.push_back(
                "Timeline includes suspicious_activity observations (" +
                suspiciousDescriptions.front() + ").");
        }
        else
        {
            hints.push_back(
                "Timeline includes suspicious_activity observations.");
        }
    }

    return hints;
}

} // namespace process
