# Data Transfer Metadata

## Purpose

Provide a reusable metadata model representing network data-transfer activity and bandwidth statistics as part of Data Transfer Analysis (Milestone M4).

## Responsibilities

- Store process ID and process name when available
- Store remote IP and remote port
- Store transport protocol information
- Store uploaded and downloaded byte totals together
- Store upload and download transfer rates
- Store total transferred bytes
- Store observation timestamps (`first_observed`, `last_updated`)
- Provide readable, deterministic formatted output
- Remain free of OS networking logic so detection and reporting modules can reuse it

## Data Model

```text
DataTransferMetadata
├── process_id
├── process_name
├── remote_ip
├── remote_port
├── protocol
├── uploaded_bytes
├── downloaded_bytes
├── upload_rate_bps
├── download_rate_bps
├── first_observed
└── last_updated
```

`total_transferred_bytes` is derived as `uploaded_bytes + downloaded_bytes`.

## Components

| Component | Location | Role |
|-----------|----------|------|
| `DataTransferMetadata` | `src/network/models/` | Portable transfer + bandwidth metadata record |
| `Connection` | `src/network/models/` | Source snapshot for `fromConnection` |
| `ConnectionBandwidthStats` | `src/network/models/` | Source for `fromBandwidthStats` |
| `NetworkUtils` | `src/network/utils/` | Protocol labels, port checks, byte/rate formatting |

## Usage

```cpp
network::Connection connection(
    4120,
    "192.168.1.10",
    53142,
    "104.18.32.45",
    443,
    network::ProtocolType::TCP,
    network::ConnectionState::ESTABLISHED,
    std::chrono::system_clock::now());

auto metadata = network::DataTransferMetadata::fromConnection(
    connection,
    13107200ull,              // uploaded
    3355443ull,               // downloaded
    1.4 * 1024.0 * 1024.0,    // upload rate
    320.0 * 1024.0,           // download rate
    "python.exe");

std::cout << metadata.toString() << "\n";
```

Or construct from existing bandwidth statistics:

```cpp
auto metadata = network::DataTransferMetadata::fromBandwidthStats(stats);
```

Or construct directly:

```cpp
network::DataTransferMetadata metadata(
    4120,
    "python.exe",
    "104.18.32.45",
    443,
    network::ProtocolType::TCP,
    13107200ull,
    3355443ull,
    1.4 * 1024.0 * 1024.0,
    320.0 * 1024.0,
    std::chrono::system_clock::now(),
    std::chrono::system_clock::now());
```

## Expected Output

```text
Process:
python.exe

Protocol:
TCP

Remote:
104.18.32.45:443

Uploaded:
12.5 MB

Downloaded:
3.2 MB

Upload Rate:
1.4 MB/s

Download Rate:
320.0 KB/s

Total Transfer:
15.7 MB
```

Listening / bind-only rows omit the `:port` suffix when the remote port is unavailable.

## Edge Cases

| Case | Behavior |
|------|----------|
| Upload + download together | Both volumes stored; total is their sum |
| Missing process name | Formatted as `Unknown` |
| UDP / TCP | Protocol stored via `ProtocolType` |
| Listening socket (remote port 0) | Remote printed as IP only (no `:0`) |
| Built from bandwidth stats | All identity and metric fields copied |

## Tests

```bash
c++ -std=c++17 -o data_transfer_metadata_tests tests/network/DataTransferMetadataTests.cpp
./data_transfer_metadata_tests
```
