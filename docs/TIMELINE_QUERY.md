# Timeline Query

## Purpose

Provide simple query helpers for timeline and short-term history data as part of Process Behavior Timeline (Milestone M5).

## Responsibilities

- Query events by process ID
- Query events by process name
- Query events by remote IP or domain
- Query events by activity type
- Query the recent N events
- Return chronological results and remain safe on empty input

## Components

| Component | Location | Role |
|-----------|----------|------|
| `TimelineQuery` | `src/process/tracker/` | Filter helpers over timeline event collections |
| `TimelineEventHistory` | `src/process/tracker/` | Optional in-memory source |
| `ProcessTimelineBuilder` | `src/process/tracker/` | Optional sorted timeline source |
| `ProcessActivityEvent` | `src/process/events/` | Queried timeline records |

## Usage

```cpp
process::TimelineEventHistory history;
history.add(events);

process::TimelineQuery query(history);

auto processTimeline = query.byProcessId(4120);
auto byName = query.byProcessName("python.exe");
auto byRemote = query.byRemoteDestination("104.18.32.45");
auto uploads = query.byEventType(process::ProcessActivityEventType::UPLOAD_ACTIVITY);
auto latest = query.recent(10);
```

Or query a plain event list / built timeline:

```cpp
process::TimelineQuery fromList(events);
process::TimelineQuery fromBuilder(builder);
```

## Query Behavior

| API | Match rule |
|-----|------------|
| `byProcessId(pid)` | Exact PID |
| `byProcessName(name)` | Exact process name |
| `byRemoteDestination(value)` | Exact endpoint, host portion of `host:port`, substring in remote address, or description text |
| `byEventType(type)` | Exact `ProcessActivityEventType` |
| `recent(n)` | Newest `n` events (`n = 0` returns all), chronological order |

## Edge Cases

| Case | Behavior |
|------|----------|
| Empty source | All queries return empty vectors |
| Unknown PID / name / remote | Empty result |
| Host-only IP query | Matches `IP:port` remote endpoints |
| Domain in remote or description | Matched by `byRemoteDestination` |

## Tests

```bash
c++ -std=c++17 -o timeline_query_tests tests/process/TimelineQueryTests.cpp
./timeline_query_tests
```
