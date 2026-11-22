# Development and verification

## Repository layout and scope

- `include/wireforge/tftp/codec.h`: separate ACK, DATA, and ERROR packet structures and decoder contracts, and validated initialization helpers.
- `src/protocol/codec.c`: ACK/DATA/ERROR validation, field decoding, and packet initialization. Private field helpers in `src/protocol/codec_helpers.h` accept the start of a validated buffer and return the opcode, block number, or error code; they are not public API.
- `include/utils/byte_order.h`: guarded header for big-endian reads; requires two readable bytes. The private codec helper header also provides `has_opcode`, which checks null input and minimum length before reading.
- `tests/unit/test_codec_ack.c`: nine named ACK tests and their command-line dispatcher.
- `tests/unit/test_codec_packet.c`: ACK/DATA initialization and DATA contract checks.
- `tests/unit/test_codec_error.c`: twelve named ERROR tests and their command-line dispatcher.
- `CMakeLists.txt`: three C99 test executables and 33 individually reported CTest entries.
- `run-tests.sh`: POSIX configure/build/test helper; forwards arguments to CTest.
- `.github/workflows/ci.yml`: Ubuntu 24.04 GCC/Clang build and test matrix, triggered by pushes, pull requests, and manual runs.

There are no third-party runtime libraries or package lockfiles. Build tools and compilers are supplied by the environment. CI configuration is not itself evidence that a particular remote run passed. Compiler warning flags are configured for GCC and Clang families; warnings are not treated as errors.

## Build, select, and troubleshoot tests

Follow the [README build instructions](../README.md#build-and-test). List test names with `ctest -N` in the build directory. Run one test with `ctest -C Debug --output-on-failure -R '^decode_ack_rejects_invalid_opcode$'`. Each test executable requires exactly one test name; invoking it without a name or with an unknown name fails.

On Windows, use a Visual Studio developer shell with CMake and the C++ build tools installed (the project itself uses C). If CMake is not on PATH, invoke its installed executable by full path. Do not share a CMake cache between Windows and WSL or between generators. Use separate build directories, for example:

```sh
cmake -S . -B build/windows -DBUILD_TESTING=ON
cmake --build build/windows --config Debug
ctest --test-dir build/windows -C Debug --output-on-failure --no-tests=error
```

This last command requires CMake 3.20 or newer. With CMake 3.16, change into `build/windows` and run `ctest -C Debug --output-on-failure` instead.

## ACK requirements and evidence

The IDs below are project identifiers used in source comments, not identifiers assigned by the RFC. Wire-format rules derive from [RFC 1350 section 5](https://www.rfc-editor.org/rfc/rfc1350#section-5); pointer validation and failure preservation are local API guarantees. This is a focused codec matrix, not a complete protocol compliance matrix.

| ID | Behavior | Existing CTest evidence |
|---|---|---|
| ACK-FR-003 | Exactly four bytes | `decode_ack_rejects_invalid_length`, `decode_ack_preserves_output_on_failure` |
| ACK-FR-005 | Opcode bytes must be `00 04` | `decode_ack_rejects_invalid_opcode`, `decode_ack_preserves_output_on_failure` |
| ACK-FR-006 | Decode block number in network byte order | `decode_ack_reads_block_in_network_byte_order` |
| ACK-FR-007 | Represent all 16-bit block numbers | `decode_ack_accepts_boundary_block_numbers`, byte-order cases; not an exhaustive enumeration |
| ACK-SR-001 | Reject null input | `decode_ack_rejects_null_input_pointer`, `decode_ack_rejects_null_input_pointer_with_zero_length`, `decode_ack_rejects_both_null_pointers` |
| ACK-SR-002 | Reject null output | `decode_ack_rejects_null_output_pointer`, `decode_ack_rejects_both_null_pointers` |
| ACK-SR-003 | Validate before reading packet bytes | Source inspection plus truncated-input cases in `decode_ack_preserves_output_on_failure`; no sanitizer proof |
| ACK-SR-004 | Preserve output on rejection | `decode_ack_preserves_output_on_failure` |

Tests exercise the decoder independently of transfer state. Acceptance of ACK block zero does not implement the initial WRQ exchange. Full specification inventory, errata disposition, session requirements, interoperability, and fault-injection evidence remain outstanding.

## DATA requirements and evidence

| IDs | Behavior | CTest evidence |
|---|---|---|
| DATA-FR-002/003/009 | Packet lengths 4 through 516; payload lengths 0 through 512 | `decode_data_validates_lengths_and_preserves_payload`, `populate_data_packet_validates_lengths` |
| DATA-FR-004 | Both opcode bytes validated | `decode_data_rejects_invalid_opcode` |
| DATA-FR-005/006 | Host-order 16-bit block | `decode_data_validates_lengths_and_preserves_payload` checks 0xabcd; full boundary enumeration is not tested for DATA |
| DATA-FR-007/008/010 | Borrow payload at offset 4, correct length, preserve bytes | `decode_data_validates_lengths_and_preserves_payload`, `decode_data_accepts_empty_payload` |
| DATA-SR-001/002 | Null input/output rejected | `decode_data_rejects_null_input`, `decode_data_rejects_null_output` |
| DATA-SR-003/006 | Validate bounds before reads and borrowed access | Source inspection and length cases; no sanitizer proof |
| DATA-SR-004/005 | Preserve fields on rejection and input bytes | `decode_data_validates_lengths_and_preserves_payload`, invalid-opcode and null-input tests |
| Local initialization contract | Validate then clear decoded fields; preserve destination on failure | `populate_data_packet_initializes_fields`, `populate_data_packet_preserves_output_on_failure`, `populate_data_packet_rejects_null_output` |

ACK initialization is covered separately by `populate_ack_packet_initializes_fields`,
`populate_ack_packet_preserves_output_on_failure`, and
`populate_ack_packet_rejects_null_output`. Table-driven cases remain inside their
named test functions; CTest reports one result per function.

## ERROR decoding

ERROR decoding follows [RFC 1350 section 5, Figure 5-4](https://www.rfc-editor.org/rfc/rfc1350#section-5).
The decoder borrows the message without netascii validation or conversion and
preserves unknown 16-bit codes. Empty messages are accepted and no DATA-size
limit applies. Local strict framing requires the first NUL to be the final byte.

| Behavior | CTest evidence |
|---|---|
| Code byte order, borrowed message, unchanged input | `decode_error_reads_code_and_message` |
| Empty and long messages | `decode_error_accepts_empty_message`, `decode_error_accepts_long_message` |
| Known and unknown codes | `decode_error_accepts_known_and_unknown_codes` |
| Null output | `decode_error_rejects_null_output`, `populate_error_packet_rejects_null_output` |
| Null input and short headers | `error_packet_rejects_null_input`, `error_packet_rejects_truncated_header` |
| Both opcode bytes | `error_packet_rejects_invalid_opcode` |
| Final terminator, no trailing bytes | `error_packet_rejects_missing_terminator`, `error_packet_rejects_trailing_bytes` |
| Both entry points preserve every field on failure | All `error_packet_rejects_*` tests |
| Validated initialization | `populate_error_packet_initializes_fields` |

Local verification on November 21, 2026 of the ERROR implementation now committed as `63df8a1fbab578c4af51b6925b87337d2321bcfa`: WSL build via `sh run-tests.sh`;
all 33 individually registered tests passed, including 12 ERROR tests.
Existing unused RRQ/WRQ constants and an ACK test initializer still produce warnings.
This does not verify session-level ERROR handling or interoperability.

## Known implementation limits

Historical verification (not the current implementation) on November 21, 2026: implementation revision `2d3d362266e1cbcbc299aeda8ba87d329a982886`, Windows x64, Visual Studio 18 2026 generator, MSVC 19.51.36243.0, Windows SDK 10.0.26100.0. Configured with `BUILD_TESTING=ON` in `build/windows-doc-sync`, built Debug, and ran `ctest --test-dir build/windows-doc-sync -C Debug --output-on-failure`: all ten tests passed. The build reported no compiler warnings. GCC/Clang, sanitizers, and remote CI were not verified in this run.

`decode_data` uses `codec_data_packet`; `decode_ack` uses `codec_ack_packet`. DATA decoding validates the standard DATA length and opcode, and decodes the block number. DATA-FR-007 and DATA-FR-008 are implemented: payload points to buffer + 4 and payload_length equals length - 4. The named packet contract tests check every payload length from 0 through 512, an exact four-byte empty packet, byte preservation, unchanged fields on rejection, and clearing DATA payload fields during initialization. The named packet contract tests cover population, DATA length boundaries, null pointers, invalid opcodes, and preservation on failure.

codec_data_payload_is_valid retains its pointer-and-length interface and is called during DATA validation. It accepts payload lengths from zero through 512.

There is no UDP transport, transfer state machine, packet encoding, RRQ/WRQ decoding or session-level ERROR handling, negotiated option support, file access, client, or server. No installable library or application target exists. Disabling `BUILD_TESTING` leaves no implementation targets.

## Contribution and release expectations

Keep changes focused, document the affected RFC section or local API rule, add relevant regression tests, and update this matrix and the README when behavior changes. Run the documented build and all tests; report toolchain, revision, failures, warnings, and tests not run. Distinguish verified codec behavior from planned transfer and extension behavior.

The API is preliminary and no stable compatibility promise or automated release/package workflow is defined. Before a release, maintainers must choose a versioning policy, document supported platforms and breaking changes, and record revision-specific build and test evidence. There is no release command to run today.

When working through the WireForge hub, commit and publish protocol changes in this repository first, then record the resulting submodule commit in the hub. Documentation edits inside this checkout are not included by committing only the hub pointer.
