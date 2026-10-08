# Timeline Event History

## Purpose

Store recent process timeline events in short-term in-memory history as part of Process Behavior Timeline (Milestone M5).

## Responsibilities

- Keep recent `ProcessActivityEvent` records available in memory
- Enforce a maximum event limit so memory does not grow forever
- Clean up events that exceed a configured maximum age
- Support history lookup by process ID or process name
- Remain safe when history is empty

## Components

| Component | Location | Role |
|-----------|----------|------|
| `TimelineEventHistory` | `src/process/tracker/` | Bounded in-memory event history |
| `ProcessActivityEvent` | `src/process/events/` | Stored timeline observations |
| `TimelineEventCollector` | `src/process/tracker/` | Optional source for bulk `add()` |

## Retention Policy

| Setting | Default | Behavior |
|---------|---------|----------|
| `maxEvents` | `1000` | Oldest events are dropped when the limit is exceeded |
| `maxAge` | `3600s` | Events older than this are removed on cleanup (`0` disables age expiry) |

`cleanup()` runs automatically after `add()` / `setMaxEvents()` / `setMaxAge()`.

## Usage

```cpp
process::TimelineEventHistory history(/*maxEvents*/ 1000,
                                      /*maxAge*/ std::chrono::seconds(3600));

history.add(event);
history.add(collector); // optional bulk ingest

auto byPid = history.getByProcessId(4120);
auto byName = history.getByProcessName("python.exe");
auto recent = history.getRecent(50);
```

## Lookup

| API | Result |
|-----|--------|
| `getByProcessId(pid)` | Chronological events for that PID |
| `getByProcessName(name)` | Chronological events with exact process name match |
| `getRecent(limit)` | Newest events in chronological order (`limit=0` returns all) |

## Edge Cases

| Case | Behavior |
|------|----------|
| Empty history | Lookups return empty vectors |
| Over maxEvents | Oldest events removed first |
| Over maxAge | Expired events removed on cleanup |
| `maxEvents = 0` | Treated as `1` |
| `maxAge = 0` | Age-based cleanup disabled |

## Tests

```bash
c++ -std=c++17 -o timeline_event_history_tests tests/process/TimelineEventHistoryTests.cpp
./timeline_event_history_tests
```
