#pragma once

#include <cstdint>
#include <string>

namespace process
{

/**
 * @brief Lifecycle status of a tracked process.
 */
enum class ProcessStatus
{
    RUNNING,
    TERMINATED
};

/**
 * @brief Represents a single process and its metadata.
 *
 * Shared across Process Monitor, Process Tracker, and network attribution.
 * Stores metadata only — no OS enumeration logic.
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
     * @param status Current process status.
     */
    ProcessInfo(
        uint32_t processId,
        const std::string& processName,
        uint32_t parentProcessId,
        ProcessStatus status = ProcessStatus::RUNNING);

    /// Process identifier.
    uint32_t processId = 0;

    /// Executable name (e.g. python.exe).
    std::string processName;

    /// Parent process identifier (0 if unavailable).
    uint32_t parentProcessId = 0;

    /// Current process status.
    ProcessStatus status = ProcessStatus::RUNNING;

    /**
     * @brief Formats process metadata for logging / inspection.
     *
     * Example:
     *   PID: 4120
     *   Process: python.exe
     *   Status: Running
     */
    std::string toString() const;
};

} // namespace process
