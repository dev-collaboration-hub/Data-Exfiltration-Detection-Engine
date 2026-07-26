#include "ProcessTracker.h"

#include <chrono>
#include <unordered_set>

namespace process
{

std::vector<ProcessEvent> ProcessTracker::update(
    const std::vector<ProcessInfo>& currentProcesses)
{
    std::vector<ProcessEvent> events;
    const auto now = std::chrono::system_clock::now();

    std::unordered_map<uint32_t, ProcessInfo> nextTracked;
    nextTracked.reserve(currentProcesses.size());

    std::unordered_set<uint32_t> seen;
    seen.reserve(currentProcesses.size());

    for (ProcessInfo processInfo : currentProcesses)
    {
        if (!seen.insert(processInfo.processId).second)
        {
            continue; // Skip duplicate PIDs within the same snapshot.
        }

        processInfo.status = ProcessStatus::RUNNING;

        // Refresh metadata for processes that are still active.
        nextTracked.emplace(processInfo.processId, processInfo);
    }

    // First snapshot only establishes baseline state (no create spam).
    if (!seeded_)
    {
        trackedProcesses_ = std::move(nextTracked);
        seeded_ = true;
        return events;
    }

    // Detect newly created processes.
    for (const auto& [pid, processInfo] : nextTracked)
    {
        if (trackedProcesses_.find(pid) == trackedProcesses_.end())
        {
            events.emplace_back(ProcessEventType::CREATED, processInfo, now);
        }
    }

    // Detect terminated processes (present previously, missing now).
    for (const auto& [pid, previous] : trackedProcesses_)
    {
        if (nextTracked.find(pid) == nextTracked.end())
        {
            ProcessInfo terminated = previous;
            terminated.status = ProcessStatus::TERMINATED;
            events.emplace_back(ProcessEventType::TERMINATED, terminated, now);
        }
    }

    trackedProcesses_ = std::move(nextTracked);
    return events;
}

const std::unordered_map<uint32_t, ProcessInfo>&
ProcessTracker::getTrackedProcesses() const
{
    return trackedProcesses_;
}

const ProcessInfo* ProcessTracker::getProcess(uint32_t processId) const
{
    const auto it = trackedProcesses_.find(processId);
    if (it == trackedProcesses_.end())
    {
        return nullptr;
    }
    return &it->second;
}

void ProcessTracker::reset()
{
    trackedProcesses_.clear();
    seeded_ = false;
}

} // namespace process
