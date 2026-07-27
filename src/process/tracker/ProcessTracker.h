#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../events/ProcessEvent.h"
#include "../models/ProcessInfo.h"

namespace process
{

/**
 * @brief Tracks active process IDs and metadata across snapshots.
 *
 * Compares each ProcessMonitor snapshot against the previous state to
 * detect process creation and termination for network attribution.
 */
class ProcessTracker
{
public:
    /**
     * @brief Compare a new process snapshot against the previous state.
     *
     * @param currentProcesses Latest processes from ProcessMonitor.
     * @return Events for newly created and terminated processes.
     */
    std::vector<ProcessEvent> update(const std::vector<ProcessInfo>& currentProcesses);

    /// Currently tracked active (running) processes keyed by PID.
    const std::unordered_map<uint32_t, ProcessInfo>& getTrackedProcesses() const;

    /// Look up metadata for a specific PID (nullptr if not tracked).
    const ProcessInfo* getProcess(uint32_t processId) const;

    /// Clears stored process state (next update will re-seed silently).
    void reset();

private:
    std::unordered_map<uint32_t, ProcessInfo> trackedProcesses_;
    bool seeded_ = false;
};

} // namespace process
