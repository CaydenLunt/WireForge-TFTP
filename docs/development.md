# Development and verification

## Repository layout and scope

- `include/wireforge/tftp/codec.h`: public ACK, DATA, ERROR, and shared request packet structures, five decoder entry points, and their validated initialization helpers.
- `src/protocol/decoder.c`: ACK/DATA/ERROR/RRQ/WRQ validation, field decoding, and packet initialization. Packet-specific rules stay in their decoder regions; RRQ/WRQ share a request parsing region.
- `src/protocol/helpers.c`: shared length/opcode validation and bounded string parsing, declared in `include/utils/codec_helpers.h`. That header also retains inline field readers. These helpers are implementation details rather than the public codec API.
- `include/utils/byte_order.h`: guarded header for big-endian reads; requires two readable bytes. The private codec helper header also provides `has_opcode`, which checks null input and minimum length before reading.
- `tests/unit/test_codec_ack.c`: nine named ACK tests and their command-line dispatcher.
- `tests/unit/test_codec_packet.c`: ACK/DATA initialization and DATA contract checks.
- `tests/unit/test_codec_error.c`: twelve named ERROR tests and their command-line dispatcher.
- `tests/unit/test_codec_request.c`: 25 named cases per request type, reported as 50 separate `rrq_*` and `wrq_*` CTest entries.
- `CMakeLists.txt`: four C99 test executables and 83 individually reported CTest entries.
- `run-tests.sh`: POSIX configure/build/test helper; forwards arguments to CTest.
- `.github/workflows/ci.yml`: Ubuntu 24.04 GCC/Clang build and test matrix, triggered by pushes, pull requests, and manual runs.

There are no third-party runtime libraries or package lockfiles. Build tools and compilers are supplied by the environment. CI configuration is not itself evidence that a particular remote run passed. Compiler warning flags are configured for GCC and Clang families; warnings are not treated as errors.

## Build, select, and troubleshoot tests

Follow the [README build instructions](../README.md#build-and-test). List test names with `ctest -N` in the build directory. Run one test with `ctest -C Debug --output-on-failure -R '^decode_ack_rejects_invalid_opcode$'`. The ACK, packet, and ERROR executables require exactly one test name. The request executable requires a request type and case name, for example `./test_codec_request rrq rejects_options`. Missing arguments, unknown request types, and unknown case names fail. Select one request case through CTest with `-R '^rrq_rejects_options$'`, or all request cases with `-R '^(rrq|wrq)_'`.

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
The ERROR IDs below match the requirements block and inline references in
`src/protocol/decoder.c`. They distinguish wire-format requirements from local API
policies. Bounded-read claims also rely on source inspection; these tests are not
sanitizer or exhaustive memory-safety evidence.

| IDs | Behavior | CTest evidence |
|---|---|---|
| ERROR-FR-003/004/006/008, ERROR-SR-005/006 | Code byte order, borrowed message, unchanged input | `decode_error_reads_code_and_message` |
| ERROR-FR-007/009 | Empty and long messages | `decode_error_accepts_empty_message`, `decode_error_accepts_long_message` |
| ERROR-FR-007 | Known and unknown codes | `decode_error_accepts_known_and_unknown_codes` |
| ERROR-SR-002 | Null output | `decode_error_rejects_null_output`, `populate_error_packet_rejects_null_output` |
| ERROR-FR-001, ERROR-SR-001/003 | Null input and short headers | `error_packet_rejects_null_input`, `error_packet_rejects_truncated_header` |
| ERROR-FR-002 | Both opcode bytes | `error_packet_rejects_invalid_opcode` |
| ERROR-FR-004/005, ERROR-SR-003 | Final terminator, no trailing bytes | `error_packet_rejects_missing_terminator`, `error_packet_rejects_trailing_bytes` |
| ERROR-SR-004 | Both entry points preserve every field on failure | All `error_packet_rejects_*` tests |
| Local initialization contract | Validated initialization | `populate_error_packet_initializes_fields` |

Historical verification on November 21, 2026 of revision `980a26235b7d648400c8e762bdca5d2e66df77cf`: WSL build via `sh run-tests.sh`;
all 33 individually registered tests passed, including 12 ERROR tests.
This build reported unused RRQ/WRQ constant warnings. The existing ACK test initializer warning can also appear when that test source is rebuilt.
This does not verify session-level ERROR handling or interoperability.

## Request decoding and evidence

RRQ and WRQ use `codec_request_block`, including the input `buffer` and `length`. Their wrappers select opcode 1 or 2; the shared request parser validates both opcode bytes, then borrows nonempty filename and mode strings. Their lengths exclude terminators. The mode terminator must be the final byte. Option pairs are currently rejected. The decoder preserves mode bytes, including case, and does not select a transfer mode or validate filesystem paths.

CTest registers each case separately for RRQ and WRQ. In the table below, replace `<request>` with `rrq` or `wrq`:

| Behavior | CTest evidence |
|---|---|
| Successful decode and initialization | `<request>_initializes_and_decodes_fields`: clear prefilled decoded fields, then verify opcode, borrowed pointers and lengths, mixed-case mode, unchanged input bytes |
| Pointer rejection | `<request>_rejects_null_decode_output`, `<request>_rejects_null_populate_output`, `<request>_rejects_null_input`, `<request>_rejects_null_input_zero_length` |
| Opcode rejection | `<request>_rejects_high_opcode_byte`, `<request>_rejects_data_opcode`, `<request>_rejects_other_request_opcode` |
| Truncation | `<request>_rejects_truncated_prefix_0` through `<request>_rejects_truncated_prefix_9`, each independently registered |
| String framing | `<request>_rejects_empty_filename`, `<request>_rejects_missing_filename_terminator`, `<request>_rejects_missing_mode`, `<request>_rejects_empty_mode`, `<request>_rejects_missing_mode_terminator` |
| Strict packet ending | `<request>_rejects_trailing_bytes`, `<request>_rejects_options` |
| Failure preservation | Every malformed-input case checks that both decode and populate leave the destination unchanged |

These 50 request entries join nine ACK, twelve packet API/DATA, and twelve ERROR entries for 83 tests total. They do not establish session handling, arbitrary-length request coverage, supported-mode policy, or sanitizer evidence.

## Decoder organization

`decoder.c` has separate ACK, DATA, ERROR, shared request helper, RRQ, and WRQ regions. Validation completes before decoded fields are assigned.

Request population calls `codec_request_buffer_is_valid` and initializes the destination without invoking a decoder or constructing decoded fields. Request decoding calls the same validator, reuses the validated filename terminator, and then assigns the decoded fields. Both calls independently validate the borrowed buffer, so a populate-then-decode sequence still scans its strings twice. The validator requires the mode terminator to end the packet, explicitly rejecting unsupported RFC 2347 options.

`helpers.c` holds repeated wire mechanics:

- `codec_buffer_is_valid` checks an inclusive length range and both opcode bytes. ACK passes an exact four-byte range; DATA passes 4 through 516; ERROR and requests use their minimum lengths with `SIZE_MAX` as the upper bound.
- `codec_string_end` bounds the first-NUL search by buffer length and offset. Its final-byte option is used for ERROR messages and request modes. Empty strings are permitted by the helper; request parsing rejects them explicitly.

The old `codec_data_payload_is_valid` helper was removed. DATA now validates the complete packet length directly. Inline opcode, block-number, error-code, and byte-order readers remain in the utility headers. These internal helpers do not replace the public validation contracts.

Compile both `src/protocol/decoder.c` and `src/protocol/helpers.c` when integrating the codec. CMake's `codec_sources` list supplies both to each test executable. No `encoder.c` exists yet.

## Verification history

On November 22, 2026, the request validation and test-registration changes now committed as `73bd330b393ed47faa17c6de78526f699e15704d` were configured and built in Debug through WSL using `sh protocols/wireforge-tftp/run-tests.sh` from the hub root. The environment uses CMake 3.28.3 and Ubuntu GCC 13.3.0. All 83 tests passed. During this documentation review, `ctest --test-dir protocols/wireforge-tftp/build --output-on-failure` passed all 83 again. The incremental build reported no compiler warnings; this was not a clean rebuild of unchanged test sources. MSVC, Clang, sanitizers, and remote CI were not verified for this revision.

On November 22, 2026, the decoder/helper refactor was built in Debug with the Visual Studio 18 2026 generator, MSVC 19.51.36243.0, and Windows SDK 10.0.26100.0 in `build/windows-requests`. All 34 CTest entries passed with no compiler warnings reported. During the documentation review at revision `b59fc2ac81925050f294eae980406087fd25d59b`, the existing build's tests were rerun with `ctest --test-dir build/windows-requests -C Debug --output-on-failure`; all 34 passed again. GCC/Clang, sanitizers, and remote CI were not run during this review.

Historical verification (not the current implementation) on November 21, 2026: implementation revision `2d3d362266e1cbcbc299aeda8ba87d329a982886`, Windows x64, Visual Studio 18 2026 generator, MSVC 19.51.36243.0, Windows SDK 10.0.26100.0. Configured with `BUILD_TESTING=ON` in `build/windows-doc-sync`, built Debug, and ran `ctest --test-dir build/windows-doc-sync -C Debug --output-on-failure`: all ten tests passed. The build reported no compiler warnings. GCC/Clang, sanitizers, and remote CI were not verified in this run.

## Known implementation limits

There is no UDP transport, transfer state machine, packet encoding or session-level ERROR handling, negotiated option support, file access, client, or server. No installable library or application target exists. Disabling `BUILD_TESTING` leaves no implementation targets.

## Contribution and release expectations

Keep changes focused, document the affected RFC section or local API rule, add relevant regression tests, and update this matrix and the README when behavior changes. Run the documented build and all tests; report toolchain, revision, failures, warnings, and tests not run. Distinguish verified codec behavior from planned transfer and extension behavior.

The API is preliminary and no stable compatibility promise or automated release/package workflow is defined. Before a release, maintainers must choose a versioning policy, document supported platforms and breaking changes, and record revision-specific build and test evidence. There is no release command to run today.

When working through the WireForge hub, commit and publish protocol changes in this repository first, then record the resulting submodule commit in the hub. Documentation edits inside this checkout are not included by committing only the hub pointer.
