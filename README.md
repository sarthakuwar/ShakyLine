<div align="center">

# ShakyLine

### Programmable Network Fault Injection Proxy

Deterministic chaos for TCP systems, built with C++20 and standalone Asio.

[![CI](https://github.com/sarthakuwar/ShakyLine/actions/workflows/ci.yml/badge.svg)](https://github.com/sarthakuwar/ShakyLine/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/sarthakuwar/ShakyLine?logo=github&label=latest)](https://github.com/sarthakuwar/ShakyLine/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![CodeQL](https://github.com/sarthakuwar/ShakyLine/actions/workflows/codeql.yml/badge.svg)](https://github.com/sarthakuwar/ShakyLine/security/code-scanning)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](#platform-support)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-informational)](#build-from-source)

</div>

---

## Download

Latest release: [ShakyLine v1.0.0](https://github.com/sarthakuwar/ShakyLine/releases/tag/v1.0.0)

The Next.js documentation and download portal lives in [`website/`](website/). Deploy that directory directly on Vercel.

| Package | Use this when | Download |
| --- | --- | --- |
| Windows installer | You want `shakyline` installed and added to `PATH` automatically. | [shakyline-1.0.0-windows-x64.exe](https://github.com/sarthakuwar/ShakyLine/releases/download/v1.0.0/shakyline-1.0.0-windows-x64.exe) |
| Windows portable ZIP | You want to extract the binary and run it without an installer. | [shakyline-1.0.0-windows-x64.zip](https://github.com/sarthakuwar/ShakyLine/releases/download/v1.0.0/shakyline-1.0.0-windows-x64.zip) |
| SHA256 checksums | You want to verify the downloaded Windows package. | [shakyline-1.0.0-windows-checksums.txt](https://github.com/sarthakuwar/ShakyLine/releases/download/v1.0.0/shakyline-1.0.0-windows-checksums.txt) |

After installing, open a new terminal and verify:

```powershell
shakyline --version
```

## Build From Source

Requirements:

| Tool | Version |
| --- | --- |
| CMake | 3.14 or newer |
| Compiler | C++20-capable compiler: MSVC 2019+, GCC 10+, or Clang 10+ |
| Git | Any current Git release |

```bash
git clone https://github.com/sarthakuwar/ShakyLine.git
cd ShakyLine
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run the built binary:

```bash
./build/shakyline --version
```

On Windows with the Visual Studio generator, the executable is usually at:

```powershell
.\build\Release\shakyline.exe --version
```

## Quick Start

Start the proxy and forward port `8080` to an upstream service:

```bash
shakyline --listen 0.0.0.0:8080 --upstream api.example.com:443 --control 9090
```

In another terminal, inject latency and packet loss:

```bash
curl -X POST http://localhost:9090/profiles/default \
  -H "Content-Type: application/json" \
  -d '{"c2s_latency_ms": 200, "c2s_drop_rate": 0.05, "s2c_latency_ms": 100}'
```

Useful control endpoints:

```bash
curl http://localhost:9090/health
curl http://localhost:9090/metrics
curl http://localhost:9090/sessions
curl -X DELETE http://localhost:9090/profiles/default
```

## What It Does

| Capability | Description |
| --- | --- |
| Deterministic faults | Reproducible chaos via SplitMix64 RNG and configurable seeds. |
| Directional profiles | Apply different behavior to client-to-server and server-to-client traffic. |
| Runtime control API | Update fault profiles while the proxy is running. |
| Prometheus metrics | Export counters and histograms from `/metrics`. |
| Correct TCP semantics | Preserve half-close behavior, graceful shutdown, and backpressure handling. |
| Async reactor model | Strand-per-session serialization keeps I/O predictable. |

## Fault Profile Fields

| Field | Type | Description |
| --- | --- | --- |
| `c2s_latency_ms` | `uint32` | Client-to-server fixed latency. |
| `c2s_jitter_ms` | `uint32` | Client-to-server random latency variance. |
| `c2s_drop_rate` | `float` | Client-to-server drop probability from `0` to `1`. |
| `c2s_throttle_kbps` | `uint32` | Client-to-server bandwidth limit in kbps. |
| `c2s_stall_prob` | `float` | Client-to-server stall probability from `0` to `1`. |
| `s2c_*` | same as `c2s_*` | Equivalent server-to-client fields. |
| `latency_ms` | `uint32` | Convenience shorthand applied to both directions. |
| `drop_rate` | `float` | Convenience shorthand applied to both directions. |

Example profile:

```json
{
  "c2s_latency_ms": 200,
  "c2s_jitter_ms": 30,
  "c2s_drop_rate": 0.05,
  "s2c_latency_ms": 100
}
```

## Command Line Options

| Option | Description | Default |
| --- | --- | --- |
| `--listen HOST:PORT` | Listen address. | `0.0.0.0:8080` |
| `--upstream HOST:PORT` | Upstream target. | `127.0.0.1:9000` |
| `--control PORT` | Control API port. | `9090` |
| `--seed NUMBER` | RNG seed for reproducible runs. | random |
| `--version`, `-v` | Print version and exit. | - |
| `--help`, `-h` | Show help and exit. | - |

## Architecture

```text
Client
  |
  v
ProxyServer  <--- accept loop
  |
  v
Session      <--- strand-serialized async I/O
  |
  +-- read buffer
  +-- delay queue
  +-- write buffer
  |
  v
AnomalyEngine <--- deterministic SplitMix64 RNG
  |
  v
Upstream Server
```

## Platform Support

CI builds and smoke-tests ShakyLine on Windows, Ubuntu, and macOS. The published binary packages currently target Windows x64.

## Contributing

Contributions are welcome. Please open an issue for significant changes, send pull requests against `main`, and update `CHANGELOG.md` when user-facing behavior changes.

See [`.github/pull_request_template.md`](.github/pull_request_template.md) for the PR checklist.

## License

[MIT](LICENSE) (c) sarthakuwar
