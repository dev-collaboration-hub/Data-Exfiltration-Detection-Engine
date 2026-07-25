#include "EventDispatcher.h"

namespace network
{

void EventDispatcher::subscribe(EventHandler handler)
{
    if (handler)
    {
        handlers_.push_back(std::move(handler));
    }
}

void EventDispatcher::dispatch(const std::vector<ConnectionEvent>& events)
{
    for (const ConnectionEvent& event : events)
    {
        dispatch(event);
    }
}

void EventDispatcher::dispatch(const ConnectionEvent& event)
{
    for (const EventHandler& handler : handlers_)
    {
        handler(event);
    }
}

void EventDispatcher::clear()
{
    handlers_.clear();
}

} // namespace network
