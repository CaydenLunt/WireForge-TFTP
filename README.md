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

The current implementation covers ACK packet decoding only. The remaining protocol behavior and extensions are planned; no complete RFC compliance or interoperability claim is made.

## Implemented Functionality

The C [ACK decoder](src/protocol/codec.c), declared in [codec.h](include/wireforge/tftp/codec.h), provides:

- Validation of the four-byte packet length and ACK opcode (`00 04`).
- Rejection of null input and output pointers.
- Decoding of the 16-bit block number in network byte order, including 0 and 65535.
- Preservation of the output value when decoding fails.

The [unit tests](tests/unit/test_codec_ack.c) cover byte order, boundary block numbers, malformed lengths and opcodes, null pointers, and unchanged output on rejection. These are codec tests; there is no UDP transport, transfer state machine, client, or server yet.

DATA decoding is in progress. `decode_data` is currently an empty stub. The internal `codec_data_payload_is_valid` helper checks for a non-null payload pointer and a payload length of 0 through 512 bytes, excluding the packet header. It does not validate the opcode and is not yet called by the decoder or covered by the ACK tests.

## Build and Test

Requirements: CMake 3.16 or newer, a C99-capable C compiler, and the build tool required by your CMake generator. Run these commands from this repository's root, or from `protocols/wireforge-tftp` when working in the WireForge hub:

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
cd build
ctest -C Debug --output-on-failure
cd ..
```

These commands also work in PowerShell when the compiler and build tools are available in the shell. `CMAKE_BUILD_TYPE` selects Debug for single-configuration generators; `--config Debug` and `-C Debug` select it for multi-configuration generators.

In a POSIX shell, the [test runner](run-tests.sh) performs the same configure, build, and test steps:

```sh
sh run-tests.sh
sh run-tests.sh -V
```

The runner resolves paths relative to itself, uses `build/`, and forwards extra arguments to CTest. Build output in `build/` is ignored by Git. CMake defines one executable target, `test_codec_ack`, with a separate CTest entry for each of its nine test functions. Each test reports its own result and can be selected by name, for example `sh run-tests.sh -R '^decode_ack_rejects_invalid_opcode$'`. `BUILD_TESTING` defaults to `ON`; disabling it currently leaves no implementation build targets. There is no standalone library, client, or server target yet.

## ACK Decoder API

```c
#include "wireforge/tftp/codec.h"

const uint8_t packet[] = {0x00, 0x04, 0x12, 0x34};
struct codec_ack_block ack = {0};
bool decoded = decode_ack(packet, sizeof(packet), &ack);
/* decoded is true; ack.block_number is 0x1234 (4660). */
```

`decode_ack` returns `true` after writing the decoded block number, or `false` for a null pointer, a length other than four, or an opcode other than ACK. On failure, a supplied output structure is unchanged. The caller must provide readable input storage for the supplied length and a valid writable output structure; non-null pointers alone do not establish memory validity. The decoder does not check whether the block number is expected by a transfer session.

For direct integration, compile `src/protocol/codec.c` with your program and add `include/` to the header search path. The API is preliminary.

## Planned Features

### Core Protocol

- Read requests (RRQ) and write requests (WRQ)
- DATA and ERROR packet encoding and decoding, plus ACK encoding
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

- Packet serialization and parsing tests beyond the existing ACK decoder tests
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

**Status: Initial Development — ACK decoding and unit tests**

ACK decoding and its CMake/CTest unit test target are implemented. The features under Planned Features remain development goals, including complete file transfers and protocol extensions.

The focused unit tests do not establish protocol-wide compliance, interoperability, or production readiness. Transfer integration tests, fault-injection tests, and a complete requirements compliance matrix remain future work.

## Contributing

Contributions, bug reports, interoperability findings, and technical discussions are welcome.

Contributions should reference relevant specifications where applicable and include tests demonstrating expected behavior.

## License

WireForge TFTP is licensed under the MIT License. See [LICENSE](LICENSE) for details.
