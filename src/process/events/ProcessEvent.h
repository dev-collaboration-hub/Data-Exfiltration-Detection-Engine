#pragma once

#include <chrono>
#include <string>

#include "../models/ProcessInfo.h"

namespace process
{

enum class ProcessEventType
{
    CREATED,
    TERMINATED
};

/**
 * @brief Represents a process lifecycle change detected by the tracker.
 */
class ProcessEvent
{
public:
    ProcessEvent() = default;

    ProcessEvent(
        ProcessEventType type,
        const ProcessInfo& processInfo,
        std::chrono::system_clock::time_point timestamp);

    ProcessEventType type = ProcessEventType::CREATED;
    ProcessInfo processInfo;
    std::chrono::system_clock::time_point timestamp;

    /**
     * @brief Formats the event for logging / inspection.
     *
     * Example:
     *   PID: 4120
     *   Process: python.exe
     *   Status: Running
     */
    std::string toString() const;
};

} // namespace process
