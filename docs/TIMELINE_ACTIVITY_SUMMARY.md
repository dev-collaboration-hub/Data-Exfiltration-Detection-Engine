# Timeline Activity Summary

## Purpose

Convert raw process timeline events into a simple human-readable activity summary as part of Process Behavior Timeline (Milestone M5) historical analysis.

## Responsibilities

- Generate a process activity summary (name, active duration)
- Summarize total upload and download volumes
- Report remote connection / endpoint counts
- Identify the most active destination
- Attach soft suspicious-looking behavior hints for M6 context
- Remain free of final security alert generation

## Components

| Component | Location | Role |
|-----------|----------|------|
| `ProcessActivitySummary` | `src/process/models/` | Portable summary record + readable formatting |
| `TimelineSummaryBuilder` | `src/process/tracker/` | Aggregates timeline events into summaries |
| `ProcessActivityEvent` | `src/process/events/` | Raw timeline observations |
| `TimelineEventHistory` | `src/process/tracker/` | Optional short-term event source |
| `ProcessTimelineBuilder` | `src/process/tracker/` | Optional sorted timeline source |

## Data Flow

```text
ProcessActivityEvent[]
        │
        ▼
TimelineSummaryBuilder
        │
        ├── forProcessId(pid)      → ProcessActivitySummary
        ├── forProcessName(name)   → ProcessActivitySummary
        └── forAllProcesses()      → ProcessActivitySummary[]
                │
                └── toSummaryString()
```

## Usage

```cpp
process::TimelineSummaryBuilder builder(events);

auto summary = builder.forProcessId(4120);
std::cout << summary.toSummaryString() << "\n";
```

Or from short-term history / a built timeline:

```cpp
process::TimelineSummaryBuilder fromHistory(history);
process::TimelineSummaryBuilder fromTimeline(timelineBuilder);
```

## Example Summary

```text
python.exe was active for 12 minutes.
It connected to 3 remote endpoints.
Total upload: 25.9 MB.
Total download: 1.2 MB.
Most active destination: api.example.com:443.
```

Optional soft hints (when heuristics match):

```text
Hint: Elevated outbound transfer volume observed.
Hint: Outbound transfer is high relative to inbound traffic.
```

## Aggregation Rules

| Metric | Rule |
|--------|------|
| Active duration | `lastSeen - firstSeen` |
| Remote endpoints | Distinct non-empty `remoteAddress` values |
| Connections opened | Count of `connection_opened` events |
| Upload / download totals | Peak bytes per remote endpoint, then summed |
| Most active destination | Remote with highest upload+download volume |

## Behavior Hints (not alerts)

Hints are descriptive context only. M5 does **not** emit final detection alerts.

| Condition | Hint |
|-----------|------|
| Upload ≥ 10 MiB | Elevated outbound transfer volume observed |
| Upload ≥ 1 MiB and ≥ 5× download | Outbound transfer is high relative to inbound traffic |
| ≥ 8 distinct remotes | Connected to many distinct remote endpoints |
| ≥ 5 upload events | Repeated upload activity observed over the timeline |
| Any `suspicious_activity` event | Timeline includes suspicious_activity observations |

## Edge Cases

| Case | Behavior |
|------|----------|
| Empty input | Empty summary; `toSummaryString()` explains no activity |
| Unknown PID / name | Empty summary |
| Growing cumulative byte counters | Peak per destination is kept (not double-counted) |
| Missing process name | Summary falls back to `PID <id>` |

## Tests

```bash
c++ -std=c++17 -o timeline_summary_builder_tests \
  tests/process/TimelineSummaryBuilderTests.cpp
./timeline_summary_builder_tests
```
