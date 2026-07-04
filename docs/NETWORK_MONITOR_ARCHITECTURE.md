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