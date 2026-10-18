#include "TimelineQuery.h"

#include <algorithm>

namespace process
{

TimelineQuery::TimelineQuery(std::vector<ProcessActivityEvent> events)
    : events_(std::move(events))
{
}

TimelineQuery::TimelineQuery(const TimelineEventHistory& history)
    : events_(history.getRecent())
{
}

TimelineQuery::TimelineQuery(const ProcessTimelineBuilder& builder)
    : events_(builder.getGlobalTimeline())
{
}

std::vector<ProcessActivityEvent> TimelineQuery::byProcessId(
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
    return sortedCopy(std::move(matched));
}

std::vector<ProcessActivityEvent> TimelineQuery::byProcessName(
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
    return sortedCopy(std::move(matched));
}

std::vector<ProcessActivityEvent> TimelineQuery::byRemoteDestination(
    const std::string& remoteIpOrDomain) const
{
    std::vector<ProcessActivityEvent> matched;
    if (remoteIpOrDomain.empty())
    {
        return matched;
    }

    for (const ProcessActivityEvent& event : events_)
    {
        if (matchesRemoteDestination(event, remoteIpOrDomain))
        {
            matched.push_back(event);
        }
    }
    return sortedCopy(std::move(matched));
}

std::vector<ProcessActivityEvent> TimelineQuery::byEventType(
    ProcessActivityEventType eventType) const
{
    std::vector<ProcessActivityEvent> matched;
    for (const ProcessActivityEvent& event : events_)
    {
        if (event.eventType == eventType)
        {
            matched.push_back(event);
        }
    }
    return sortedCopy(std::move(matched));
}

std::vector<ProcessActivityEvent> TimelineQuery::recent(std::size_t count) const
{
    std::vector<ProcessActivityEvent> ordered = sortedCopy(events_);
    if (count == 0 || count >= ordered.size())
    {
        return ordered;
    }

    return std::vector<ProcessActivityEvent>(
        ordered.end() - static_cast<std::ptrdiff_t>(count),
        ordered.end());
}

std::size_t TimelineQuery::size() const
{
    return events_.size();
}

bool TimelineQuery::earlierThan(
    const ProcessActivityEvent& left,
    const ProcessActivityEvent& right)
{
    if (left.timestamp != right.timestamp)
    {
        return left.timestamp < right.timestamp;
    }

    if (left.processId != right.processId)
    {
        return left.processId < right.processId;
    }

    return static_cast<int>(left.eventType) < static_cast<int>(right.eventType);
}

bool TimelineQuery::matchesRemoteDestination(
    const ProcessActivityEvent& event,
    const std::string& remoteIpOrDomain)
{
    if (event.remoteAddress == remoteIpOrDomain)
    {
        return true;
    }

    const std::string eventHost = hostFromEndpoint(event.remoteAddress);
    const std::string queryHost = hostFromEndpoint(remoteIpOrDomain);

    if (!eventHost.empty() && eventHost == queryHost)
    {
        return true;
    }

    // Allow partial host queries such as "104.18.32" or "api.example".
    if (!event.remoteAddress.empty() &&
        event.remoteAddress.find(remoteIpOrDomain) != std::string::npos)
    {
        return true;
    }

    if (!event.description.empty() &&
        event.description.find(remoteIpOrDomain) != std::string::npos)
    {
        return true;
    }

    return false;
}

std::string TimelineQuery::hostFromEndpoint(const std::string& endpoint)
{
    if (endpoint.empty())
    {
        return {};
    }

    // IPv6 literals may appear as [2001:db8::1]:443
    if (endpoint.front() == '[')
    {
        const auto closing = endpoint.find(']');
        if (closing != std::string::npos)
        {
            return endpoint.substr(1, closing - 1);
        }
        return endpoint;
    }

    // Prefer the last ':' so IPv4 host:port splits cleanly without breaking
    // bare IPv6 addresses that contain multiple colons and no brackets.
    const auto colon = endpoint.rfind(':');
    if (colon == std::string::npos)
    {
        return endpoint;
    }

    const std::string maybeHost = endpoint.substr(0, colon);
    const std::string maybePort = endpoint.substr(colon + 1);
    if (maybePort.empty())
    {
        return endpoint;
    }

    for (char ch : maybePort)
    {
        if (ch < '0' || ch > '9')
        {
            return endpoint; // Not a trailing numeric port.
        }
    }

    return maybeHost;
}

std::vector<ProcessActivityEvent> TimelineQuery::sortedCopy(
    std::vector<ProcessActivityEvent> events)
{
    std::stable_sort(events.begin(), events.end(), earlierThan);
    return events;
}

} // namespace process
