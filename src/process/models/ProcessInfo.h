#pragma once

#include <cstdint>
#include <string>

namespace process
{

/**
 * @brief Represents a single running process on the local system.
 *
 * This model stores process metadata only and does not contain
 * any operating system enumeration logic.
 */
class ProcessInfo
{
public:
    ProcessInfo() = default;

    /**
     * @brief Parameterized constructor.
     *
     * @param processId Process ID (PID).
     * @param processName Executable name (e.g. python.exe).
     * @param parentProcessId Parent process ID, or 0 if unavailable.
     */
    ProcessInfo(
        uint32_t processId,
        const std::string& processName,
        uint32_t parentProcessId);

    /// Process identifier.
    uint32_t processId = 0;

    /// Executable name (e.g. python.exe).
    std::string processName;

    /// Parent process identifier (0 if unavailable).
    uint32_t parentProcessId = 0;
};

} // namespace process
