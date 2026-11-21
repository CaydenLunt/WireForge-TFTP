# Development and verification

## Repository layout and scope

- `include/wireforge/tftp/codec.h`: shared packet structure, ACK/DATA decoder contracts, and validated initialization helpers.
- `src/protocol/codec.c`: ACK/DATA validation, block-number decoding, and packet initialization.
- `tests/unit/test_codec_ack.c`: nine named ACK tests and their command-line dispatcher.
- `tests/unit/test_codec_packet.c`: shared initialization and DATA contract checks.
- `CMakeLists.txt`: two C99 test executables and ten CTest entries.
- `run-tests.sh`: POSIX configure/build/test helper; forwards arguments to CTest.
- `.github/workflows/ci.yml`: Ubuntu 24.04 GCC/Clang build and test matrix, triggered by pushes, pull requests, and manual runs.

There are no third-party runtime libraries or package lockfiles. Build tools and compilers are supplied by the environment. CI configuration is not itself evidence that a particular remote run passed. Compiler warning flags are configured for GCC and Clang families; warnings are not treated as errors.

## Build, select, and troubleshoot tests

Follow the [README build instructions](../README.md#build-and-test). List test names with `ctest -N` in the build directory. Run one test with `ctest -C Debug --output-on-failure -R '^decode_ack_rejects_invalid_opcode$'`. The executable requires exactly one test name; invoking it without a name or with an unknown name fails.

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

## Known implementation limits

Local verification on November 19, 2026: refactor working tree, Windows x64, Visual Studio 18 2026 generator, MSVC 19.51.36243.0, Windows SDK 10.0.26100.0. Configured with `BUILD_TESTING=ON` in `build/packet-refactor`, built Debug, and ran `ctest --test-dir build/packet-refactor -C Debug --output-on-failure`: all ten tests passed. The build reported no compiler warnings. GCC/Clang, sanitizers, and remote CI were not verified in this run.

decode_data now uses codec_packet, validates the standard DATA length and opcode, and decodes the block number. Payload extraction remains deferred. The codec_packet_contract test covers population, DATA length boundaries, null pointers, invalid opcodes, and preservation on failure.

codec_data_payload_is_valid retains its pointer-and-length interface and is called during DATA validation. It accepts payload lengths from zero through 512.

There is no UDP transport, transfer state machine, packet encoding, RRQ/WRQ/ERROR handling, negotiated option support, file access, client, or server. No installable library or application target exists. Disabling `BUILD_TESTING` leaves no implementation targets.

## Contribution and release expectations

Keep changes focused, document the affected RFC section or local API rule, add relevant regression tests, and update this matrix and the README when behavior changes. Run the documented build and all tests; report toolchain, revision, failures, warnings, and tests not run. Never describe planned DATA behavior as verified.

The API is preliminary and no stable compatibility promise or automated release/package workflow is defined. Before a release, maintainers must choose a versioning policy, document supported platforms and breaking changes, and record revision-specific build and test evidence. There is no release command to run today.

When working through the WireForge hub, commit and publish protocol changes in this repository first, then record the resulting submodule commit in the hub. Documentation edits inside this checkout are not included by committing only the hub pointer.
