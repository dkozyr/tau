# Tau WebRTC C++ Library

A lightweight, modular WebRTC implementation in modern C++ focused on simplicity and control. This library provides all core components required to establish and maintain a peer-to-peer WebRTC connection, including ICE, DTLS, SRTP, RTP, RTCP, H.264 support, and SDP negotiation

Designed for **bundle-only** sessions to minimize complexity and accelerate connection setup

---

## Key Features

### ICE (Interactive Connectivity Establishment)

* Socket-agnostic, callback-based design
* Supports host, server reflexive (STUN), and relay (TURN) candidates
* One UDP port per local interface (shared across all connections)
* mDNS client is to enhance privacy by masking local IP addresses during peer connection setup
* **Note**: TURN over TCP is currently **not supported**

See also [ICE readme](ice/README.md) and [mDNS readme](mdns/README.md)

### DTLS + SRTP

* DTLS handshake and encryption/decryption via **OpenSSL**
* SRTP and SRTCP encryption via **Cisco libsrtp**
* Seamlessly integrated into media transport layer

### RTP / RTCP

* Full support for RTP/RTCP transport, packet parsing, and handling
* Built-in support for RTCP Sender/Receiver Reports, NACK

### H.264 Packetizer / Depacketizer

* Fragmentation and reassembly of H.264 video over RTP

### SDP Parser and Negotiation

* Lightweight SDP reader and writer
* Simplified negotiation mechanism tailored to bundle-only use

### Boost ASIO-based Networking

* All socket operations handled via **Boost ASIO**
* Thread-safe async I/O
* Clean separation of networking and transport layers

## Limitations

* No TURN-over-TCP (UDP-only)
* No support for non-bundled SDP configurations

## Contributing

Contributions are welcome! Please open issues or submit PRs for bugfixes or feature requests
