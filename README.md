

## 🧪 Anomaly Client Toolkit

### ⚡ Simulate real-world network conditions (TCP/UDP) for testing server robustness.

---

### 📦 Features

* ✅ TCP & UDP anomaly injection clients
* 🔁 Custom TCP handshake (`SYN → SYN-CUSTACK → ACK-CUSTOM`)
* 🧱 Simulates:

  * Packet **loss**
  * Packet **corruption**
  * **Duplication**
  * **Out-of-order** delivery (TCP only)
  * Artificial **delay**
* 🔀 Multi-threaded TCP server for concurrent testing
* 🧰 Simple CLI-based usage for integration into test pipelines

---

## 🛠️ Build Instructions

### Requirements

* CMake ≥ 3.10
* g++ or clang++
* Linux/macOS

### Build

```bash
git clone https://github.com/yourusername/anomaly-client-toolkit.git
cd anomaly-client-toolkit
mkdir build && cd build
cmake ..
make
sudo make install  # optional: installs binaries to /usr/local/bin
```

---

## 🚀 CLI Tools

| Binary                        | Description                          |
| ----------------------------- | ------------------------------------ |
| `anomaly_tcp`                 | TCP anomaly injection client         |
| `anomaly_udp`                 | UDP anomaly injection client         |
| `anomaly_tcp_server`          | Basic TCP echo server with handshake |
| `anomaly_udp_server`          | Basic UDP echo server                |
| `anomaly_threaded_tcp_server` | Multi-client threaded TCP server     |

---

## 🧪 Example Usage

### 1. Start the Test Server

```bash
anomaly_threaded_tcp_server
```

Or:

```bash
anomaly_udp_server
```

---

### 2. Run TCP Anomaly Client

```bash
anomaly_tcp -l 0.3 -c -d -o -t 200
```

**Flags:**

| Flag        | Description                        |
| ----------- | ---------------------------------- |
| `-l <rate>` | Packet loss rate (e.g., 0.2 = 20%) |
| `-c`        | Corrupt data                       |
| `-d`        | Duplicate packets                  |
| `-o`        | Out-of-order send (TCP only)       |
| `-t <ms>`   | Add delay before each packet       |
| `-f`        | Custom TCP handshake override      |
| `-h <host>` | Server IP (default: 127.0.0.1)     |
| `-p <port>` | Server port (default: 12345)       |

---

### 3. Run UDP Anomaly Client

```bash
anomaly_udp -l 0.5 -c -d -t 100
```

---

## 🧰 Sample Output

**Client:**

```
[+] Connected to server.
[>] Sent: SYN
[<] Server: SYN-CUSTACK
[>] Sent: ACK-CUSTOM
[>] Sent: Hello
[!] Packet dropped (simulated): Hello
[!] Duplicate packet sent
```

**Server:**

```
🚀 TCP Server listening on port 12345...
[+] Client connected!
[<] Received: SYN
[<] Received: ACK-CUSTOM
[<] Received: Hello
[<] Received: Hello
```

---

## 🧪 Use Cases

* Stress testing your server under unreliable network conditions
* Verifying retry logic and fault tolerance
* Integration with CI/CD testing pipelines
* Educational tool for learning TCP/UDP robustness

---

## 🤝 Contributing

Feel free to submit PRs to:

* Add configurable test message files
* Improve corruption logic
* Support for TLS/QUIC
* Metrics and logging

---



