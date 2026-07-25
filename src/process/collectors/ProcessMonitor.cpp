#include "ProcessMonitor.h"

#include <string>

#include <windows.h>
#include <tlhelp32.h>

namespace process
{

namespace
{
class SnapshotHandle
{
public:
    explicit SnapshotHandle(HANDLE handle)
        : handle_(handle)
    {
    }

    ~SnapshotHandle()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle_);
        }
    }

    SnapshotHandle(const SnapshotHandle&) = delete;
    SnapshotHandle& operator=(const SnapshotHandle&) = delete;

    HANDLE get() const
    {
        return handle_;
    }

    bool valid() const
    {
        return handle_ != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle_;
};
} // namespace

std::vector<ProcessInfo> ProcessMonitor::getActiveProcesses()
{
    std::vector<ProcessInfo> processes;

    // Snapshot can fail under low resources or access restrictions.
    SnapshotHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot.valid())
    {
        return processes;
    }

    // Use ANSI Toolhelp APIs so executable names are plain char*
    // (e.g. "python.exe") regardless of UNICODE build settings.
    PROCESSENTRY32A entry{};
    entry.dwSize = sizeof(PROCESSENTRY32A);

    if (!Process32FirstA(snapshot.get(), &entry))
    {
        return processes;
    }

    do
    {
        processes.emplace_back(
            static_cast<uint32_t>(entry.th32ProcessID),
            std::string(entry.szExeFile),
            static_cast<uint32_t>(entry.th32ParentProcessID));
    } while (Process32NextA(snapshot.get(), &entry));

    return processes;
}

} // namespace process
