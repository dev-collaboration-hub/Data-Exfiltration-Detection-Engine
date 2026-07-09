# Network Monitor Architecture

## Purpose

The Network Monitor is responsible for collecting and monitoring endpoint network activity. It gathers active network connections from the operating system and provides structured network telemetry to higher-level components such as the Correlation Engine and Detection Engine.

The module focuses solely on network data collection and connection lifecycle tracking. Detection logic, behavioral analysis, and alert generation are handled by separate modules to maintain a clear separation of responsibilities.

## Responsibilities

The Network Monitor is responsible for:

- Monitoring active network connections.
- Detecting newly established and terminated connections.
- Collecting connection metadata such as:
  - Process ID (PID)
  - Local IP Address
  - Remote IP Address
  - Port
  - Protocol
  - Connection State
  - Timestamp
- Maintaining an up-to-date view of endpoint network activity.
- Publishing connection events to downstream components.

---

## Proposed Module Structure

```text
network/
│
├── interfaces/
│     INetworkMonitor.h
│
├── models/
│     Connection.h
│
├── collectors/
│     NetworkMonitor.h
│     NetworkMonitor.cpp
│
├── tracker/
│     ConnectionTracker.h
│     ConnectionTracker.cpp
│
├── events/
│     EventDispatcher.h
│     EventDispatcher.cpp
│
└── utils/
      NetworkUtils.h
      NetworkUtils.cpp
```

### Module Description

| Module | Responsibility |
|---------|----------------|
| interfaces | Defines contracts for network monitoring components. |
| models | Stores data structures representing network connections. |
| collectors | Collects network connection information from the operating system. |
| tracker | Tracks connection lifecycle (new, active, closed). |
| events | Publishes connection events to other system components. |
| utils | Common helper utilities shared across the module. |

---

## Core Components

### NetworkMonitor

The main component responsible for collecting active network connections from the operating system.

Responsibilities:

- Query Windows networking APIs.
- Collect active TCP/UDP connections.
- Pass collected data to the Connection Tracker.

---

### Connection

Represents a single network connection.

Example fields:

- Process ID (PID)
- Process Name
- Local Address
- Remote Address
- Port
- Protocol
- Connection State
- Timestamp

---

### ConnectionTracker

Maintains the current state of active network connections.

Responsibilities:

- Detect newly established connections.
- Detect closed connections.
- Prevent duplicate connection records.

---

### EventDispatcher

Publishes connection events so downstream modules can consume them without directly depending on the Network Monitor.

Example consumers include:

- Correlation Engine
- Detection Engine
- Logging components

---

## High-Level Data Flow

```text
Windows Networking APIs
          │
          ▼
    Network Monitor
          │
          ▼
   Connection Tracker
          │
          ▼
    Event Dispatcher
          │
          ▼
  Correlation Engine
          │
          ▼
   Detection Engine
```

### Flow Description

1. The operating system exposes active network connection information.
2. The Network Monitor collects connection data.
3. The Connection Tracker maintains the current connection state.
4. Connection events are published through the Event Dispatcher.
5. Higher-level modules consume these events for correlation, behavioral analysis, and threat detection.

---

## Architecture Decisions

### 1. Separation of Responsibilities

The Network Monitor is responsible only for collecting network telemetry. Threat detection, behavioral analysis, and alert generation are handled by separate modules.

**Reason:**
Keeping responsibilities separate makes the system easier to maintain, test, and extend.

---

### 2. Event-Driven Communication

The Network Monitor publishes connection events instead of directly invoking downstream components.

**Reason:**
This reduces coupling between modules and allows multiple components (Correlation Engine, Detection Engine, Logging, etc.) to consume the same events independently.

---

### 3. Modular Design

The module is divided into logical components such as collectors, trackers, models, and events.

**Reason:**
This improves code organization, readability, and future scalability.

---

### 4. Platform Abstraction

Platform-specific network collection logic should remain isolated inside the Network Monitor implementation.

**Reason:**
Higher-level components should remain independent of operating system APIs, making future platform support easier.

---

## Future Enhancements

Future iterations of the Network Monitor may include:

- IPv6 support.
- Real-time event streaming.
- Connection history caching.
- Domain name resolution.
- Upload and download traffic statistics.
- Connection filtering.
- Performance optimizations for high connection volumes.