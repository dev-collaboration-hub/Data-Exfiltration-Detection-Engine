#include "ProcessActivityEvent.h"

#include <ctime>
#include <iomanip>
#include <sstream>

#include "../../network/utils/NetworkUtils.h"

namespace process
{

ProcessActivityEvent::ProcessActivityEvent(
    std::chrono::system_clock::time_point timestamp,
    uint32_t processId,
    const std::string& processName,
    ProcessActivityEventType eventType,
    const std::string& localAddress,
    const std::string& remoteAddress,
    network::ProtocolType protocol,
    uint64_t uploadedBytes,
    uint64_t downloadedBytes,
    const std::string& description)
    : timestamp(timestamp),
      processId(processId),
      processName(processName),
      eventType(eventType),
      localAddress(localAddress),
      remoteAddress(remoteAddress),
      protocol(protocol),
      uploadedBytes(uploadedBytes),
      downloadedBytes(downloadedBytes),
      description(description)
{
}

std::string ProcessActivityEvent::eventTypeToString(ProcessActivityEventType type)
{
    switch (type)
    {
    case ProcessActivityEventType::PROCESS_SEEN:
        return "process_seen";
    case ProcessActivityEventType::CONNECTION_OPENED:
        return "connection_opened";
    case ProcessActivityEventType::UPLOAD_ACTIVITY:
        return "upload_activity";
    case ProcessActivityEventType::DOWNLOAD_ACTIVITY:
        return "download_activity";
    case ProcessActivityEventType::CONNECTION_CLOSED:
        return "connection_closed";
    case ProcessActivityEventType::SUSPICIOUS_ACTIVITY:
        return "suspicious_activity";
    default:
        return "unknown";
    }
}

std::string ProcessActivityEvent::defaultDescription(ProcessActivityEventType type)
{
    switch (type)
    {
    case ProcessActivityEventType::PROCESS_SEEN:
        return "Process Started";
    case ProcessActivityEventType::CONNECTION_OPENED:
        return "Network Connection Opened";
    case ProcessActivityEventType::UPLOAD_ACTIVITY:
        return "Upload Activity Detected";
    case ProcessActivityEventType::DOWNLOAD_ACTIVITY:
        return "Download Activity Detected";
    case ProcessActivityEventType::CONNECTION_CLOSED:
        return "Connection Closed";
    case ProcessActivityEventType::SUSPICIOUS_ACTIVITY:
        return "Suspicious Activity Detected";
    default:
        return "Unknown Activity";
    }
}

std::string ProcessActivityEvent::formatTimestamp() const
{
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm{};

#if defined(_WIN32)
    gmtime_s(&tm, &timeValue);
#else
    gmtime_r(&timeValue, &tm);
#endif

    std::ostringstream out;
    out << std::put_time(&tm, "%H:%M");
    return out.str();
}

std::string ProcessActivityEvent::toTimelineString() const
{
    const std::string& text =
        description.empty() ? defaultDescription(eventType) : description;

    std::ostringstream out;
    out << formatTimestamp() << " " << text;
    return out.str();
}

std::string ProcessActivityEvent::toString() const
{
    const std::string& text =
        description.empty() ? defaultDescription(eventType) : description;

    std::ostringstream out;
    out << "Timestamp:\n"
        << formatTimestamp()
        << "\n\n"
        << "PID:\n"
        << processId
        << "\n\n"
        << "Process:\n"
        << (processName.empty() ? "Unknown" : processName)
        << "\n\n"
        << "Event Type:\n"
        << eventTypeToString(eventType)
        << "\n\n"
        << "Local Address:\n"
        << localAddress
        << "\n\n"
        << "Remote Address:\n"
        << remoteAddress
        << "\n\n"
        << "Protocol:\n"
        << network::NetworkUtils::protocolToString(protocol)
        << "\n\n"
        << "Uploaded:\n"
        << network::NetworkUtils::formatBytes(uploadedBytes)
        << "\n\n"
        << "Downloaded:\n"
        << network::NetworkUtils::formatBytes(downloadedBytes)
        << "\n\n"
        << "Description:\n"
        << text;

    return out.str();
}

} // namespace process
