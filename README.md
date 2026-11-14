# WireForge TFTP

**A specification-driven implementation of the Trivial File Transfer Protocol.**

WireForge TFTP is an open-source implementation of the Trivial File Transfer Protocol (TFTP), developed from scratch using official RFC specifications.

Part of the larger **WireForge** project, this repository focuses on understanding and implementing network protocols at the packet level, emphasizing standards compliance, interoperability, reliability, and rigorous testing.

The goal is not simply to create a functional TFTP client and server, but to develop an implementation whose behavior can be verified against published specifications and tested under real-world network conditions.

## Project Goals

- **Standards Compliance:** Implement TFTP according to its published RFC specifications.
- **Interoperability:** Communicate correctly with existing TFTP clients, servers, and network boot environments.
- **Reliability:** Handle packet loss, retransmissions, timeouts, duplicate packets, and unexpected network behavior.
- **Concurrency:** Support multiple independent file transfers without interference.
- **Observability:** Provide structured logging and meaningful diagnostics for transfer activity and failures.
- **Security:** Validate incoming packets, enforce transfer boundaries, and handle malformed or unauthorized requests safely.
- **Testing:** Verify protocol behavior through unit, integration, interoperability, and fault-injection testing.

## Protocol Overview

TFTP is a lightweight file transfer protocol operating over UDP. Unlike TCP-based file transfer protocols, TFTP uses a simple lockstep acknowledgment mechanism to coordinate data transmission.

TFTP is commonly used in network boot environments, embedded systems, firmware provisioning, and network device configuration.

This project will implement the protocol directly using UDP sockets, without relying on existing TFTP protocol libraries.

## Specifications

The implementation will reference the following RFCs:

| RFC | Specification | Scope |
|---|---|---|
| [1350](https://www.rfc-editor.org/rfc/rfc1350) | The TFTP Protocol (Revision 2) | Core protocol |
| [2347](https://www.rfc-editor.org/rfc/rfc2347) | TFTP Option Extension | Option negotiation |
| [2348](https://www.rfc-editor.org/rfc/rfc2348) | TFTP Blocksize Option | Configurable block sizes |
| [2349](https://www.rfc-editor.org/rfc/rfc2349) | TFTP Timeout Interval and Transfer Size Options | Timeout and transfer-size negotiation |
| [7440](https://www.rfc-editor.org/rfc/rfc7440) | TFTP Windowsize Option | Windowed transfers |

Implementation coverage and compliance status will be documented as development progresses.

## Planned Features

### Core Protocol

- Read requests (RRQ) and write requests (WRQ)
- DATA, ACK, and ERROR packet encoding and decoding
- Binary (`octet`) and text (`netascii`) transfer modes
- Block numbering and acknowledgment handling
- Timeout detection and retransmission
- Transfer ID (TID) validation
- Transfer completion and error handling

### Client and Server

- Standalone TFTP client
- Standalone TFTP server
- Independent concurrent transfer sessions
- Configurable server settings
- Controlled file access and path validation
- Per-transfer status tracking and diagnostics

### Protocol Extensions

- RFC 2347 option negotiation
- RFC 2348 blocksize negotiation
- RFC 2349 timeout and transfer-size negotiation
- RFC 7440 windowed transfers

### Verification and Testing

- Packet serialization and parsing tests
- Transfer state-machine tests
- Client/server integration tests
- Interoperability testing with established implementations
- Packet loss, duplication, and delay simulation
- Malformed packet and unexpected TID testing
- Concurrent transfer stress testing
- Network boot interoperability testing

## Development Approach

WireForge TFTP follows a specification-driven engineering process.

Each implementation phase will:

1. Identify applicable RFC requirements.
2. Define expected protocol behavior and edge cases.
3. Implement the required packet handling and state transitions.
4. Develop automated tests covering normal and adverse conditions.
5. Validate interoperability with independent implementations.
6. Document compliance evidence, limitations, and design decisions.

The project prioritizes correctness, clarity, and testability over unnecessary abstraction.

## Project Status

**Status: Planning / Initial Development**

The implementation has not yet been released. Features described in this README represent development goals rather than completed functionality.

Build instructions, usage examples, and API documentation will be added as the implementation progresses.

## Contributing

Contributions, bug reports, interoperability findings, and technical discussions are welcome.

Contributions should reference relevant specifications where applicable and include tests demonstrating expected behavior.

## License

WireForge TFTP is licensed under the MIT License. See [LICENSE](LICENSE) for details.
