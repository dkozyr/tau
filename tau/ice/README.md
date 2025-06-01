# ICE Module

This module implements a minimal yet flexible version of the **ICE** (Interactive Connectivity Establishment) protocol, designed specifically for use in WebRTC applications with **bundle-only** sessions. The implementation is socket-agnostic and is intended to be integrated with external networking logic via callbacks

## Key Features

* **Socket-independent design**: All network I/O is abstracted via user-defined callbacks. This allows full control over socket management and I/O strategy
* **Bundle-only support**: Only bundle-only sessions are supported. All media, RTP, and RTCP packets are transmitted over a single transport connection, with demultiplexing handled by higher-level logic
* **Sorted local interface list**: A list of local host candidates (IP addresses and ports) is passed in via a `std::vector`, sorted by client-defined priority (e.g., based on network type, cost, or latency)
* **STUN support**: Queries public (server reflexive) addresses via standard STUN servers
* **TURN support**: Allows media relay through TURN servers when direct connection fails
* **Single-port-per-interface model**: For each local interface, only **one** UDP port is used for communication with all involved peers and servers (remote peer, STUN, TURN). This minimizes socket footprint and simplifies port management

## Limitations

* **No TURN over TCP**: This implementation currently only supports TURN over UDP. As a result, it **cannot establish connections** in environments that require TCP-only traversal (e.g., strict firewalls or highly restricted networks)
* **Only bundle-only**: Non-bundled or multiple-media-stream scenarios are not supported at this stage

## Network Architecture

```text
+-------------------------------------------------------------+
|                         UDP Socket                          |
| (receives all incoming packets on a single local port)      |
+------------------------------+------------------------------+
                               |
                               v
+------------------------------+------------------------------+
|                         ICE Agent                           |
|  - Filters and handles STUN messages                        |
|  - Manages connectivity checks                              |
|  - Maintains candidate pairs                                |
+------------------------------+------------------------------+
                               |
                               v
+------------------------------+------------------------------+
|                      DTLS Transport Layer                   |
|  - Handles DTLS handshake                                   |
|  - Decrypts DTLS packets                                    |
|  - Forwards RTP/RTCP payloads                               |
+------------------------------+------------------------------+
                               |
                               v
+------------------------------+------------------------------+
|                      Media Demuxer                          |
|  - Parses RTP and RTCP packets                              |
|  - Separates by SSRC / MID / PT                             |
|  - Routes to appropriate media handlers                     |
+------------+--------------------+---------------------------+
             |                    |                   
             v                    v
     +-------------+      +--------------+       +-----------------+
     | Audio Track |      | Video Track  |  ...  | DataChannel(s)  |
     +-------------+      +--------------+       +-----------------+
```
