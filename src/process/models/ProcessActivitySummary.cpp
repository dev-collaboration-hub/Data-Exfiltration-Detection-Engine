#include "ProcessActivitySummary.h"

#include <sstream>

#include "../../network/utils/NetworkUtils.h"

namespace process
{

ProcessActivitySummary::ProcessActivitySummary(
    uint32_t processId,
    const std::string& processName,
    std::chrono::system_clock::time_point firstSeen,
    std::chrono::system_clock::time_point lastSeen,
    std::size_t remoteEndpointCount,
    std::size_t connectionOpenCount,
    uint64_t totalUploadedBytes,
    uint64_t totalDownloadedBytes,
    const std::string& mostActiveDestination,
    const std::vector<std::string>& behaviorHints)
    : processId(processId)
    , processName(processName)
    , firstSeen(firstSeen)
    , lastSeen(lastSeen)
    , remoteEndpointCount(remoteEndpointCount)
    , connectionOpenCount(connectionOpenCount)
    , totalUploadedBytes(totalUploadedBytes)
    , totalDownloadedBytes(totalDownloadedBytes)
    , mostActiveDestination(mostActiveDestination)
    , behaviorHints(behaviorHints)
{
}

std::chrono::seconds ProcessActivitySummary::activeDuration() const
{
    if (firstSeen.time_since_epoch().count() == 0 &&
        lastSeen.time_since_epoch().count() == 0)
    {
        return std::chrono::seconds(0);
    }

    if (lastSeen < firstSeen)
    {
        return std::chrono::seconds(0);
    }

    return std::chrono::duration_cast<std::chrono::seconds>(lastSeen - firstSeen);
}

bool ProcessActivitySummary::empty() const
{
    return processId == 0 && processName.empty() &&
           remoteEndpointCount == 0 && connectionOpenCount == 0 &&
           totalUploadedBytes == 0 && totalDownloadedBytes == 0 &&
           mostActiveDestination.empty() && behaviorHints.empty() &&
           firstSeen.time_since_epoch().count() == 0 &&
           lastSeen.time_since_epoch().count() == 0;
}

std::string ProcessActivitySummary::formatDuration(std::chrono::seconds duration)
{
    const auto totalSeconds = duration.count();
    if (totalSeconds <= 0)
    {
        return "less than a minute";
    }

    if (totalSeconds < 60)
    {
        return std::to_string(totalSeconds) +
               (totalSeconds == 1 ? " second" : " seconds");
    }

    const long long totalMinutes = totalSeconds / 60;
    if (totalMinutes < 60)
    {
        return std::to_string(totalMinutes) +
               (totalMinutes == 1 ? " minute" : " minutes");
    }

    const long long hours = totalMinutes / 60;
    const long long minutes = totalMinutes % 60;
    std::ostringstream out;
    out << hours << (hours == 1 ? " hour" : " hours");
    if (minutes > 0)
    {
        out << " " << minutes << (minutes == 1 ? " minute" : " minutes");
    }
    return out.str();
}

std::string ProcessActivitySummary::toSummaryString() const
{
    if (empty())
    {
        return "No process activity to summarize.";
    }

    const std::string name =
        processName.empty() ? ("PID " + std::to_string(processId)) : processName;

    std::ostringstream out;
    out << name << " was active for " << formatDuration(activeDuration())
        << ".\n";

    out << "It connected to " << remoteEndpointCount << " remote endpoint"
        << (remoteEndpointCount == 1 ? "" : "s") << ".\n";

    out << "Total upload: "
        << network::NetworkUtils::formatBytes(totalUploadedBytes) << ".\n";

    out << "Total download: "
        << network::NetworkUtils::formatBytes(totalDownloadedBytes) << ".";

    if (!mostActiveDestination.empty())
    {
        out << "\nMost active destination: " << mostActiveDestination << ".";
    }

    for (const std::string& hint : behaviorHints)
    {
        out << "\nHint: " << hint;
    }

    return out.str();
}

std::string ProcessActivitySummary::toString() const
{
    std::ostringstream out;
    out << "Process:\n"
        << (processName.empty() ? "(unknown)" : processName) << "\n\n"
        << "PID:\n"
        << processId << "\n\n"
        << "Active Duration:\n"
        << formatDuration(activeDuration()) << "\n\n"
        << "Remote Endpoints:\n"
        << remoteEndpointCount << "\n\n"
        << "Connections Opened:\n"
        << connectionOpenCount << "\n\n"
        << "Uploaded:\n"
        << network::NetworkUtils::formatBytes(totalUploadedBytes) << "\n\n"
        << "Downloaded:\n"
        << network::NetworkUtils::formatBytes(totalDownloadedBytes) << "\n\n"
        << "Most Active Destination:\n"
        << (mostActiveDestination.empty() ? "(none)" : mostActiveDestination);

    if (!behaviorHints.empty())
    {
        out << "\n\nBehavior Hints:";
        for (const std::string& hint : behaviorHints)
        {
            out << "\n- " << hint;
        }
    }

    return out.str();
}

} // namespace process
