#include "TimelineEventHistory.h"

#include <algorithm>

namespace process
{

TimelineEventHistory::TimelineEventHistory(
    std::size_t maxEvents,
    std::chrono::seconds maxAge)
    : maxEvents_(maxEvents == 0 ? 1 : maxEvents),
      maxAge_(maxAge)
{
}

void TimelineEventHistory::add(const ProcessActivityEvent& event)
{
    events_.push_back(event);
    cleanup();
}

void TimelineEventHistory::add(const std::vector<ProcessActivityEvent>& events)
{
    if (events.empty())
    {
        return;
    }

    events_.insert(events_.end(), events.begin(), events.end());
    cleanup();
}

void TimelineEventHistory::add(const TimelineEventCollector& collector)
{
    add(collector.getAllEvents());
}

std::vector<ProcessActivityEvent> TimelineEventHistory::getByProcessId(
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

    std::stable_sort(matched.begin(), matched.end(), earlierThan);
    return matched;
}

std::vector<ProcessActivityEvent> TimelineEventHistory::getByProcessName(
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

    std::stable_sort(matched.begin(), matched.end(), earlierThan);
    return matched;
}

std::vector<ProcessActivityEvent> TimelineEventHistory::getRecent(
    std::size_t limit) const
{
    std::vector<ProcessActivityEvent> recent = events_;
    std::stable_sort(recent.begin(), recent.end(), earlierThan);

    if (limit == 0 || limit >= recent.size())
    {
        return recent;
    }

    // Keep the newest @p limit events, still in chronological order.
    return std::vector<ProcessActivityEvent>(
        recent.end() - static_cast<std::ptrdiff_t>(limit),
        recent.end());
}

std::size_t TimelineEventHistory::size() const
{
    return events_.size();
}

std::size_t TimelineEventHistory::maxEvents() const
{
    return maxEvents_;
}

void TimelineEventHistory::setMaxEvents(std::size_t maxEvents)
{
    maxEvents_ = maxEvents == 0 ? 1 : maxEvents;
    cleanup();
}

std::chrono::seconds TimelineEventHistory::maxAge() const
{
    return maxAge_;
}

void TimelineEventHistory::setMaxAge(std::chrono::seconds maxAge)
{
    maxAge_ = maxAge;
    cleanup();
}

std::size_t TimelineEventHistory::cleanup()
{
    const std::size_t before = events_.size();
    if (events_.empty())
    {
        return 0;
    }

    if (maxAge_.count() > 0)
    {
        const auto now = std::chrono::system_clock::now();
        events_.erase(
            std::remove_if(
                events_.begin(),
                events_.end(),
                [this, now](const ProcessActivityEvent& event) {
                    if (event.timestamp == std::chrono::system_clock::time_point{})
                    {
                        return false;
                    }
                    return now - event.timestamp > maxAge_;
                }),
            events_.end());
    }

    if (events_.size() > maxEvents_)
    {
        std::stable_sort(events_.begin(), events_.end(), earlierThan);
        const std::size_t overflow = events_.size() - maxEvents_;
        events_.erase(
            events_.begin(),
            events_.begin() + static_cast<std::ptrdiff_t>(overflow));
    }

    return before - events_.size();
}

void TimelineEventHistory::clear()
{
    events_.clear();
}

bool TimelineEventHistory::earlierThan(
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

} // namespace process
