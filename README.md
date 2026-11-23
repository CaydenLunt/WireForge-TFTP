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

The current implementation decodes ACK, DATA, ERROR, RRQ, and WRQ packets and provides validated packet initialization. Transfer handling and protocol extensions are planned; no complete RFC compliance or interoperability claim is made.

## Implemented Functionality

The C [decoders](src/protocol/decoder.c), declared in [codec.h](include/wireforge/tftp/codec.h), cover all five base packet types:

| Packet | Decoded fields | Validation |
|---|---|---|
| ACK | Block number | Opcode 4; exactly 4 bytes |
| DATA | Block number and borrowed payload | Opcode 3; 4 through 516 bytes |
| ERROR | Error code and borrowed message | Opcode 5; at least 5 bytes; message NUL ends packet |
| RRQ | Opcode, borrowed filename and mode | Opcode 1; nonempty NUL-terminated strings; mode NUL ends packet |
| WRQ | Opcode, borrowed filename and mode | Opcode 2; same string rules as RRQ |

All decode and populate functions reject null pointers and preserve destination fields on failure. Input bytes are never modified. Request option extensions are rejected; mode interpretation and filename/path policy belong to the caller.

ACK decoding specifically provides:

- Validation of the four-byte packet length and ACK opcode (`00 04`).
- Rejection of null input and output pointers.
- Decoding of the 16-bit block number in network byte order, including 0 and 65535.
- Preservation of the output value when decoding fails.

The [unit tests](tests/unit/test_codec_ack.c) cover byte order, boundary block numbers, malformed lengths and opcodes, null pointers, and unchanged output on rejection. These are codec tests; there is no UDP transport, transfer state machine, client, or server yet.

ACK uses `struct codec_ack_packet`; DATA uses `struct codec_data_packet`, which also exposes `payload` and `payload_length`. DATA decoding validates opcode 3 and lengths 4 through 516, then decodes the block number and exposes the payload at `buffer + 4` with `payload_length = length - 4`. The initialization helpers validate and initialize a borrowed buffer and length with `block_number` set to zero, leaving the structure unchanged on failure.

## Build and Test

See the [development and verification guide](docs/development.md) for repository layout, CI, requirement-to-test mapping, known compiler warnings, contribution guidance, and release limitations.

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

The runner resolves paths relative to itself, uses `build/`, and forwards extra arguments to CTest. Build output in `build/` is ignored by Git. CMake builds four test executables: `test_codec_ack` (9 tests), `test_codec_packet` (12 tests), `test_codec_error` (12 tests), and `test_codec_request` (one entry covering RRQ/WRQ cases). Each of the 34 tests has a separate CTest entry; table-driven cases within a function share its result. Each test reports its own result and can be selected by name, for example `sh run-tests.sh -R '^decode_ack_rejects_invalid_opcode$'`. `BUILD_TESTING` defaults to `ON`; disabling it currently leaves no implementation build targets. There is no standalone library, client, or server target yet.

## Packet Decoder API

```c
#include "wireforge/tftp/codec.h"

const uint8_t packet[] = {0x00, 0x04, 0x12, 0x34};
struct codec_ack_packet ack = {packet, sizeof(packet), 0};
bool decoded = decode_ack(&ack);
/* decoded is true; ack.block_number is 0x1234 (4660). */
```

`decode_ack` returns `true` after writing the decoded block number, or `false` for a null pointer, a length other than four, or an opcode other than ACK. On failure, a supplied output structure is unchanged. The caller must provide readable input storage for the supplied length and a valid writable output structure; non-null pointers alone do not establish memory validity. The decoder does not check whether the block number is expected by a transfer session.

ACK, DATA, and ERROR use their respective `codec_ack_packet`, `codec_data_packet`, and `codec_error_packet` structures. RRQ and WRQ share `codec_request_block`. Each decoder takes a pointer to its writable packet structure. `decode_data(&packet)` validates the DATA opcode and standard length before writing the host-order block number. Successful decoding leaves `buffer` and `length` unchanged; failure leaves all fields unchanged.

`populate_ack_packet(&packet, buffer, length)` and `populate_data_packet(&packet, buffer, length)` validate the appropriate opcode and length before initializing the borrowed buffer, length, and `block_number = 0`. They do not decode the block number. Null pointers or invalid packets return `false` without changing the destination. Keep the buffer alive and readable through decoding. Successful DATA decoding exposes a borrowed payload pointer at byte offset 4 and payload_length equal to packet length minus 4 (0 through 512). Keep the buffer alive while accessing the payload. Empty DATA has a one-past-end payload pointer that must not be dereferenced. DATA initialization clears payload to NULL and payload_length to zero. ACK packets have no payload fields.

ERROR uses `struct codec_error_packet`. `populate_error_packet(&packet, buffer, length)` validates opcode 5, a minimum length of five bytes, and a message whose first NUL is the final packet byte. It stores the borrowed buffer and length, setting `error_code = 0`, `error_message = NULL`, and `error_message_length = 0`. `decode_error(&packet)` performs the same validation and exposes the host-order error code and message at `buffer + 4`, with length `packet.length - 5` excluding the terminator. Both functions return `true` on success, reject null pointers, and preserve all fields on failure.

Empty messages and all 16-bit error codes, including unknown codes, are accepted. Message bytes are preserved without netascii validation or conversion. ERROR has no DATA-style 512-byte limit. No memory is allocated or copied; the input buffer must remain alive and readable while accessing the message. Do not free the borrowed message separately. Session-level interpretation of errors remains unimplemented.

```c
const uint8_t error_bytes[] = {0, 5, 0, 1, 'M', 'i', 's', 's', 'i', 'n', 'g', 0};
struct codec_error_packet error = {0};
if (populate_error_packet(&error, error_bytes, sizeof(error_bytes)) &&
    decode_error(&error)) {
    /* error.error_code == 1; error.error_message_length == 7. */
    /* error.error_message points to "Missing" inside error_bytes. */
}
```

The [ERROR tests](tests/unit/test_codec_error.c) exercise valid codes/messages, null pointers, truncated headers, invalid opcodes, missing terminators, trailing bytes, and unchanged output on failure.

### Read and write requests

`populate_rrq_packet` and `populate_wrq_packet` validate the buffer, store `buffer` and `length`, and clear `opcode`, `filename`, `filename_length`, `mode`, and `mode_length` to zero or NULL. `decode_rrq` and `decode_wrq` validate again and fill these fields. Both strings are borrowed from the input; lengths exclude their NUL terminators. Keep the input alive while accessing either string.

Requests require at least six bytes, the matching two-byte opcode, and nonempty filename and mode strings. The first mode NUL must end the packet: trailing bytes, including option pairs, are rejected. No DATA-style size cap is imposed. Mode spelling and case are preserved without checking supported modes or performing text conversion. Decoding a filename does not authorize file access or validate a filesystem path.

```c
const uint8_t request_bytes[] = {0, 1, 'f', 0, 'o', 'c', 't', 'e', 't', 0};
struct codec_request_block request = {0};
if (populate_rrq_packet(&request, request_bytes, sizeof(request_bytes)) &&
    decode_rrq(&request)) {
    /* request.opcode == 1; request.filename_length == 1. */
    /* request.mode points to "octet"; request.mode_length == 5. */
}
```

For WRQ, use opcode 2 with `populate_wrq_packet` and `decode_wrq`. The [request tests](tests/unit/test_codec_request.c) run both paths under the `codec_request_validation` CTest entry.

### Source integration

Compile [decoder.c](src/protocol/decoder.c) and [helpers.c](src/protocol/helpers.c) with your program and add `include/` to the header search path. `codec.h` remains the public interface. Shared length/opcode validation and bounded string parsing live in `helpers.c`; packet-specific rules and the shared RRQ/WRQ parser remain in named regions in `decoder.c`. Inline byte-order readers remain in the utility headers. There is no encoder implementation yet. The API is preliminary.

## Planned Features

### Core Protocol

- Read-request and write-request session handling
- RRQ, WRQ, DATA, ERROR, and ACK encoding
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

- Packet serialization and parsing tests beyond the existing ACK, DATA, ERROR, RRQ, and WRQ codec tests
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

**Status: Initial Development - ACK/DATA/ERROR/RRQ/WRQ decoding and unit tests**

ACK/DATA block decoding, ERROR code/message decoding, borrowed payload/message access, validated initialization, RRQ/WRQ request decoding, and four test executables with 34 CTest entries are implemented. The features under Planned Features remain development goals, including complete file transfers and protocol extensions.

The focused unit tests do not establish protocol-wide compliance, interoperability, or production readiness. Transfer integration tests, fault-injection tests, and a complete requirements compliance matrix remain future work.

## Contributing

Contributions, bug reports, interoperability findings, and technical discussions are welcome.

Contributions should reference relevant specifications where applicable and include tests demonstrating expected behavior.

## License

WireForge TFTP is licensed under the MIT License. See [LICENSE](LICENSE) for details.
