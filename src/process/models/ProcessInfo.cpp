#include "ProcessInfo.h"

#include <sstream>

namespace process
{

namespace
{
std::string statusToString(ProcessStatus status)
{
    switch (status)
    {
    case ProcessStatus::RUNNING:
        return "Running";
    case ProcessStatus::TERMINATED:
        return "Terminated";
    default:
        return "Unknown";
    }
}
} // namespace

ProcessInfo::ProcessInfo(
    uint32_t processId,
    const std::string& processName,
    uint32_t parentProcessId,
    ProcessStatus status)
    : processId(processId),
      processName(processName),
      parentProcessId(parentProcessId),
      status(status)
{
}

std::string ProcessInfo::toString() const
{
    std::ostringstream out;
    out << "PID: " << processId << "\n"
        << "Process: " << processName << "\n"
        << "Status: " << statusToString(status);
    return out.str();
}

} // namespace process
