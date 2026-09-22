# Timeline Event Collection

## Purpose

Collect process, connection, and transfer observations into reusable timeline events grouped by PID as part of Process Behavior Timeline (Milestone M5).

## Responsibilities

- Collect active process events from the process monitor / process tracker
- Collect connection opened and closed events from the network connection tracker
- Collect upload and download activity from transfer trackers
- Group collected events by process ID
- Avoid duplicate timeline observations
- Remain safe on empty input

## Data Flow

```text
ProcessMonitor / ProcessTracker
        │ ProcessEvent / ProcessInfo
        ▼
ConnectionTracker ──────────────┐
        │ ConnectionEvent       │
        ▼                       ▼
UploadTracker / DownloadTracker │
        │ Upload/Download stats │
        ▼                       ▼
        TimelineEventCollector
                │
                ▼
        ProcessActivityEvent[] grouped by PID
```

## Components

| Component | Location | Role |
|-----------|----------|------|
| `TimelineEventCollector` | `src/process/tracker/` | Collects, deduplicates, and groups timeline events |
| `ProcessActivityEvent` | `src/process/events/` | Reusable timeline event record |
| `ProcessEvent` | `src/process/events/` | Process lifecycle source events |
| `ConnectionEvent` | `src/network/events/` | Connection lifecycle source events |
| `ConnectionUploadStats` | `src/network/models/` | Upload transfer source records |
| `ConnectionDownloadStats` | `src/network/models/` | Download transfer source records |

## Usage

```cpp
process::TimelineEventCollector collector;

auto processEvents = processTracker.update(processes);
auto connectionEvents = connectionTracker.update(connections);
auto uploadStats = uploadTracker.update(snapshots);
auto downloadStats = downloadTracker.update(snapshots);

collector.collect(
    processEvents,
    connectionEvents,
    uploadStats,
    downloadStats,
    processTracker.getTrackedProcesses());

for (const auto& event : collector.getEventsForProcess(4120))
{
    std::cout << event.toTimelineString() << "\n";
}
```

Or seed process-seen rows directly from a monitor snapshot:

```cpp
collector.collectActiveProcesses(processMonitor.getProcesses());
```

## Deduplication

| Source | Key basis |
|--------|-----------|
| Process seen | `process_seen` + PID |
| Connection opened/closed | event type + PID + protocol + 4-tuple |
| Upload activity | PID + protocol + remote endpoint + uploaded bytes |
| Download activity | PID + protocol + remote endpoint + downloaded bytes |

Repeated collection of the same observation is ignored. Growing upload/download totals produce a new timeline entry.

## Edge Cases

| Case | Behavior |
|------|----------|
| Empty input vectors | No events added; collector remains valid |
| Multiple events for one PID | All stored under that PID |
| Network event for PID X | Attached only to process X |
| Zero-byte upload/download | Ignored (no activity yet) |
| Missing process name on connection | Filled from optional process lookup map |

## Tests

```bash
c++ -std=c++17 -o timeline_event_collector_tests tests/process/TimelineEventCollectorTests.cpp
./timeline_event_collector_tests
```
