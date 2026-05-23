# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2025-05-23

### Added
- Initial release of ShakyLine
- TCP proxy with deterministic fault injection via SplitMix64 RNG
- Fault types: Latency, Jitter, Drop, Throttle, Corrupt, Stall, Half-close
- Directional fault profiles (client→server / server→client)
- Runtime control REST API on configurable port
- Prometheus-compatible `/metrics` endpoint
- 4-way half-close with correct TCP shutdown semantics
- Backpressure handling with configurable high/low watermarks
- Graceful shutdown with buffer draining
- Cross-platform support: Windows (MSVC), Linux (GCC/Clang), macOS
- `--version` flag
- GitHub Actions CI (matrix build on Windows, Ubuntu, macOS)
- Windows NSIS installer and ZIP package via CPack

[Unreleased]: https://github.com/sarthakuwar/ShakyLine/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/sarthakuwar/ShakyLine/releases/tag/v1.0.0
