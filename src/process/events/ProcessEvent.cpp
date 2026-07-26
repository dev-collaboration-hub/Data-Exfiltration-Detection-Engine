#include "ProcessEvent.h"

namespace process
{

ProcessEvent::ProcessEvent(
    ProcessEventType type,
    const ProcessInfo& processInfo,
    std::chrono::system_clock::time_point timestamp)
    : type(type),
      processInfo(processInfo),
      timestamp(timestamp)
{
}

std::string ProcessEvent::toString() const
{
    return processInfo.toString();
}

} // namespace process
