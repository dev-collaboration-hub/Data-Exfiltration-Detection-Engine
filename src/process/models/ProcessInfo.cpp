#include "ProcessInfo.h"

namespace process
{

ProcessInfo::ProcessInfo(
    uint32_t processId,
    const std::string& processName,
    uint32_t parentProcessId)
    : processId(processId),
      processName(processName),
      parentProcessId(parentProcessId)
{
}

} // namespace process
