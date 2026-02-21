# ShakyLine — Programmable Network Fault Injection Proxy

A **production-quality cross-platform TCP proxy** for deterministic network fault injection, built with C++17 and standalone Asio.

## Features

- **Reactor pattern** with strand-per-session serialization
- **Deterministic fault injection** using SplitMix64 RNG
- **Directional profiles** — inject faults asymmetrically (client→server / server→client)
- **Runtime control API** — update profiles without restart
- **Prometheus metrics** — counters and histograms
- **4-way half-close** — correct TCP shutdown semantics
- **Backpressure handling** — high/low watermarks
- **Graceful shutdown** — drain buffers before closing

## Fault Types

| Fault | Description |
|-------|-------------|
| Latency | Add fixed delay (ms) |
| Jitter | Add random delay variance |
| Drop | Discard packets |
| Throttle | Limit bandwidth (kbps) |
| Corrupt | XOR random byte |
| Stall | Stop reading temporarily |
| Half-close | Initiate FIN |

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

**Requirements:**
- CMake 3.14+
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)

## Usage

```bash
./shakyline --listen 0.0.0.0:8080 --upstream api.example.com:443 --control 9090 --seed 12345
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--listen HOST:PORT` | Listen address | 0.0.0.0:8080 |
| `--upstream HOST:PORT` | Upstream target | 127.0.0.1:9000 |
| `--control PORT` | Control API port | 9090 |
| `--seed NUMBER` | RNG seed | random |

## Control API

### Update Profile

```bash
curl -X POST http://localhost:9090/profiles/default \
  -H "Content-Type: application/json" \
  -d '{
    "c2s_latency_ms": 200,
    "c2s_drop_rate": 0.05,
    "s2c_latency_ms": 100
  }'
```

### Get Metrics

```bash
curl http://localhost:9090/metrics
```

### List Sessions

```bash
curl http://localhost:9090/sessions
```

### Health Check

```bash
curl http://localhost:9090/health
```

### Delete Profile

```bash
curl -X DELETE http://localhost:9090/profiles/default
```

## Profile Fields

| Field | Type | Description |
|-------|------|-------------|
| `c2s_latency_ms` | uint32 | Client→Server latency |
| `c2s_jitter_ms` | uint32 | Client→Server jitter |
| `c2s_drop_rate` | float | Client→Server drop probability (0-1) |
| `c2s_throttle_kbps` | uint32 | Client→Server bandwidth limit |
| `c2s_stall_prob` | float | Client→Server stall probability |
| `s2c_*` | - | Same fields for Server→Client |
| `latency_ms` | uint32 | Both directions (convenience) |
| `drop_rate` | float | Both directions (convenience) |

## Architecture

```
Client ─────┐
            │
            ▼
    ┌───────────────┐
    │  ProxyServer  │◄── Accept loop
    └───────┬───────┘
            │
            ▼
    ┌───────────────┐
    │    Session    │◄── Strand-serialized
    │ ┌───────────┐ │
    │ │ ReadBuf   │ │    3-tier buffer:
    │ │ DelayQueue│ │    Read → Delay → Write
    │ │ WriteBuf  │ │
    │ └───────────┘ │
    └───────┬───────┘
            │
            ▼
    ┌───────────────┐
    │ AnomalyEngine │◄── Deterministic RNG
    └───────────────┘
            │
            ▼
       Upstream Server
```

## License

MIT
