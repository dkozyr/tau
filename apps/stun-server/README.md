# Simple STUN Server

This is a minimal implementation of a **STUN (Session Traversal Utilities for NAT)** server using **UDP**. It listens for STUN requests and responds with the XOR-MAPPED-ADDRESS attribute, which allows clients to discover their public IP address and port.

---

## Features

* Listens for incoming STUN messages over **UDP**
* Default port: **3478**
* Validates STUN messages
* Responds to valid **STUN Binding Requests** with a **Binding Response**
* Includes `XOR-MAPPED-ADDRESS` attribute as defined in [RFC 5389](https://datatracker.ietf.org/doc/html/rfc5389)

---

## Limitations

* `TCP` is not supported
