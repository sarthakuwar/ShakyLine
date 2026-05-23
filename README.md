<div align="center">

# ⚡ ShakyLine

### Programmable Network Fault Injection Proxy

*Deterministic chaos for your TCP stack — built with C++17 and Asio*

[![CI](https://github.com/sarthakuwar/ShakyLine/actions/workflows/ci.yml/badge.svg)](https://github.com/sarthakuwar/ShakyLine/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/sarthakuwar/ShakyLine?logo=github&label=latest)](https://github.com/sarthakuwar/ShakyLine/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![CodeQL](https://github.com/sarthakuwar/ShakyLine/actions/workflows/codeql.yml/badge.svg)](https://github.com/sarthakuwar/ShakyLine/security/code-scanning)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](#)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-informational)](#)

</div>

---

## ⬇️ Install

### Windows (Recommended)

Download the installer — it puts `shakyline` on your PATH automatically:

| Asset | Description |
|-------|-------------|
| 🖥️ **[ShakyLine-Setup.exe](https://github.com/sarthakuwar/ShakyLine/releases/latest)** | NSIS installer — installs to `Program Files`, adds to PATH |
| 📦 **[ShakyLine-windows-x64.zip](https://github.com/sarthakuwar/ShakyLine/releases/latest)** | Portable ZIP — just extract and run |

After installing, open a new terminal and verify:

```powershell
shakyline --version
```

### Build from Source

**Requirements:** CMake 3.14+, C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)

```bash
git clone https://github.com/sarthakuwar/ShakyLine.git
cd ShakyLine
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| 🔀 **Reactor pattern** | Strand-per-session serialization for safe async I/O |
| 🎲 **Deterministic faults** | Reproducible chaos via SplitMix64 RNG with configurable seed |
| ↔️ **Directional profiles** | Inject faults asymmetrically (client→server / server→client) |
| 🔌 **Runtime control API** | Update fault profiles on the fly — no restart needed |
| 📊 **Prometheus metrics** | Counters and histograms at `/metrics` |
| 🔒 **Correct TCP semantics** | 4-way half-close, graceful shutdown, backpressure watermarks |

---

## 🧨 Fault Types

| Fault | Flag | Description |
|-------|------|-------------|
| Latency | `c2s_latency_ms` | Add fixed delay (milliseconds) |
| Jitter | `c2s_jitter_ms` | Add random delay variance |
| Drop | `c2s_drop_rate` | Discard packets probabilistically |
| Throttle | `c2s_throttle_kbps` | Limit bandwidth (kbps) |
| Corrupt | — | XOR a random byte in the payload |
| Stall | `c2s_stall_prob` | Pause reading temporarily |
| Half-close | — | Initiate TCP FIN mid-stream |

---

## 🚀 Quick Start

```bash
# Start the proxy — forward port 8080 → api.example.com:443
shakyline --listen 0.0.0.0:8080 --upstream api.example.com:443 --control 9090

# In another terminal, inject 200 ms latency + 5% packet drop
curl -X POST http://localhost:9090/profiles/default \
  -H "Content-Type: application/json" \
  -d '{"c2s_latency_ms": 200, "c2s_drop_rate": 0.05, "s2c_latency_ms": 100}'

# Check live metrics (Prometheus format)
curl http://localhost:9090/metrics

# List active sessions
curl http://localhost:9090/sessions

# Health check
curl http://localhost:9090/health

# Remove the fault profile
curl -X DELETE http://localhost:9090/profiles/default
```

---

## ⚙️ Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--listen HOST:PORT` | Listen address | `0.0.0.0:8080` |
| `--upstream HOST:PORT` | Upstream target | `127.0.0.1:9000` |
| `--control PORT` | Control API port | `9090` |
| `--seed NUMBER` | RNG seed (for reproducibility) | random |
| `--version` | Print version and exit | — |
| `--help` | Show help | — |

---

## 📐 Profile Fields

| Field | Type | Description |
|-------|------|-------------|
| `c2s_latency_ms` | `uint32` | Client→Server latency |
| `c2s_jitter_ms` | `uint32` | Client→Server jitter |
| `c2s_drop_rate` | `float` | Client→Server drop probability (0–1) |
| `c2s_throttle_kbps` | `uint32` | Client→Server bandwidth limit |
| `c2s_stall_prob` | `float` | Client→Server stall probability |
| `s2c_*` | — | Same fields for Server→Client direction |
| `latency_ms` | `uint32` | Convenience shorthand — both directions |
| `drop_rate` | `float` | Convenience shorthand — both directions |

---

## 🏗️ Architecture

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
    │ AnomalyEngine │◄── Deterministic RNG (SplitMix64)
    └───────────────┘
            │
            ▼
       Upstream Server
```

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Open an issue first for significant changes
2. Fork → branch → PR against `main`
3. Ensure CI passes and `CHANGELOG.md` is updated

See [`.github/pull_request_template.md`](.github/pull_request_template.md) for the PR checklist.

---

## 📄 License

[MIT](LICENSE) © sarthakuwar
