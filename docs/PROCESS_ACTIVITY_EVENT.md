# Process Activity Event

## Purpose

Provide a reusable event structure for storing process activity on a behavior timeline as part of Process Behavior Timeline (Milestone M5).

## Responsibilities

- Store an observation timestamp on every event
- Store process ID and process name
- Store a typed activity kind
- Store local and remote endpoint addresses
- Store transport protocol
- Store uploaded and downloaded byte totals
- Store a human-readable description
- Provide clear printed output for sample timelines and inspection
- Remain free of OS monitoring and detection logic

## Event Types

| Enum | Label | Default description |
|------|-------|---------------------|
| `PROCESS_SEEN` | `process_seen` | Process Started |
| `CONNECTION_OPENED` | `connection_opened` | Network Connection Opened |
| `UPLOAD_ACTIVITY` | `upload_activity` | Upload Activity Detected |
| `DOWNLOAD_ACTIVITY` | `download_activity` | Download Activity Detected |
| `CONNECTION_CLOSED` | `connection_closed` | Connection Closed |
| `SUSPICIOUS_ACTIVITY` | `suspicious_activity` | Suspicious Activity Detected |

## Data Model

```text
ProcessActivityEvent
├── timestamp
├── process_id
├── process_name
├── event_type
├── local_address
├── remote_address
├── protocol
├── uploaded_bytes
├── downloaded_bytes
└── description
```

## Components

| Component | Location | Role |
|-----------|----------|------|
| `ProcessActivityEvent` | `src/process/events/` | Reusable timeline event record |
| `ProcessActivityEventType` | `src/process/events/` | Activity kind enumeration |
| `ProtocolType` | `src/network/models/` | Shared transport protocol enum |
| `NetworkUtils` | `src/network/utils/` | Protocol and byte formatting helpers |

## Usage

```cpp
auto when = /* 10:04 UTC */;

process::ProcessActivityEvent event(
    when,
    4120,
    "python.exe",
    process::ProcessActivityEventType::UPLOAD_ACTIVITY,
    "192.168.1.10:53142",
    "104.18.32.45:443",
    network::ProtocolType::TCP,
    13107200ull,
    3355443ull,
    "Upload Activity Detected");

std::cout << event.toTimelineString() << "\n";
std::cout << event.toString() << "\n";
```

## Expected Output

Timeline listing:

```text
10:01 Process Started

10:03 Network Connection Opened

10:04 Upload Activity Detected

10:10 Connection Closed
```

Full event dump:

```text
Timestamp:
10:04

PID:
4120

Process:
python.exe

Event Type:
upload_activity

Local Address:
192.168.1.10:53142

Remote Address:
104.18.32.45:443

Protocol:
TCP

Uploaded:
12.5 MB

Downloaded:
3.2 MB

Description:
Upload Activity Detected
```

## Edge Cases

| Case | Behavior |
|------|----------|
| Empty description | `toTimelineString` / `toString` use the default phrase for the event type |
| Empty process name | Formatted as `Unknown` |
| Missing addresses | Local / Remote sections render empty |
| Custom suspicious text | Custom `description` overrides the default phrase |

## Tests

```bash
c++ -std=c++17 -o process_activity_event_tests tests/process/ProcessActivityEventTests.cpp
./process_activity_event_tests
```
