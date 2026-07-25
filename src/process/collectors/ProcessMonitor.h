#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <vector>

#include "../models/ProcessInfo.h"

namespace process
{

/**
 * @brief Enumerates active processes on the local Windows system.
 */
class ProcessMonitor
{
public:
    /**
     * @brief Collect all currently running processes.
     *
     * On failure, returns an empty list instead of throwing.
     */
    std::vector<ProcessInfo> getActiveProcesses();
};

} // namespace process

#endif
