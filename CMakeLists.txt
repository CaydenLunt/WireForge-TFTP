cmake_minimum_required(VERSION 3.16)

project(wireforge-tftp LANGUAGES C)

# Apply strict warnings to C targets defined in this directory and below.
add_compile_options(
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>"
)

include(CTest)

set(codec_sources src/protocol/decoder.c src/protocol/helpers.c)

if(BUILD_TESTING)
    add_executable(test_codec_request tests/unit/test_codec_request.c ${codec_sources})
    target_include_directories(test_codec_request PRIVATE include)
    target_compile_features(test_codec_request PRIVATE c_std_99)
    set(codec_request_tests
        rejects_null_decode_output
        rejects_null_populate_output
        initializes_and_decodes_fields
        rejects_null_input_zero_length
        rejects_null_input
        rejects_high_opcode_byte
        rejects_data_opcode
        rejects_other_request_opcode
        rejects_empty_filename
        rejects_missing_filename_terminator
        rejects_missing_mode
        rejects_empty_mode
        rejects_missing_mode_terminator
        rejects_trailing_bytes
        rejects_options
    )
    foreach(prefix_length RANGE 0 9)
        list(APPEND codec_request_tests rejects_truncated_prefix_${prefix_length})
    endforeach()
    foreach(request_type rrq wrq)
        foreach(test_name IN LISTS codec_request_tests)
            add_test(NAME ${request_type}_${test_name}
                     COMMAND test_codec_request ${request_type} ${test_name})
        endforeach()
    endforeach()
    add_executable(test_codec_packet tests/unit/test_codec_packet.c ${codec_sources})
    target_include_directories(test_codec_packet PRIVATE include)
    target_compile_features(test_codec_packet PRIVATE c_std_99)
    set(codec_packet_tests
        populate_ack_packet_rejects_null_output
        populate_data_packet_rejects_null_output
        decode_data_rejects_null_output
        populate_ack_packet_initializes_fields
        decode_data_validates_lengths_and_preserves_payload
        populate_data_packet_validates_lengths
        decode_data_rejects_invalid_opcode
        decode_data_rejects_null_input
        populate_ack_packet_preserves_output_on_failure
        populate_data_packet_preserves_output_on_failure
        populate_data_packet_initializes_fields
        decode_data_accepts_empty_payload
    )
    foreach(test_name IN LISTS codec_packet_tests)
        add_test(NAME ${test_name} COMMAND test_codec_packet ${test_name})
    endforeach()
    add_executable(test_codec_ack
        tests/unit/test_codec_ack.c
        ${codec_sources}
    )
    target_include_directories(test_codec_ack PRIVATE include)
    target_compile_features(test_codec_ack PRIVATE c_std_99)
    set(codec_ack_tests
        decode_ack_reads_block_in_network_byte_order
        decode_ack_rejects_invalid_length
        decode_ack_rejects_invalid_opcode
        decode_ack_rejects_null_output_pointer
        decode_ack_rejects_null_input_pointer
        decode_ack_rejects_null_input_pointer_with_zero_length
        decode_ack_rejects_both_null_pointers
        decode_ack_accepts_boundary_block_numbers
        decode_ack_preserves_output_on_failure
    )
    foreach(test_name IN LISTS codec_ack_tests)
        add_test(NAME ${test_name} COMMAND test_codec_ack ${test_name})
    endforeach()
    add_executable(test_codec_error tests/unit/test_codec_error.c ${codec_sources})
    target_include_directories(test_codec_error PRIVATE include)
    target_compile_features(test_codec_error PRIVATE c_std_99)
    set(codec_error_tests
        decode_error_reads_code_and_message
        decode_error_accepts_empty_message
        decode_error_accepts_known_and_unknown_codes
        decode_error_accepts_long_message
        decode_error_rejects_null_output
        populate_error_packet_rejects_null_output
        error_packet_rejects_null_input
        error_packet_rejects_truncated_header
        error_packet_rejects_invalid_opcode
        error_packet_rejects_missing_terminator
        error_packet_rejects_trailing_bytes
        populate_error_packet_initializes_fields
    )
    foreach(test_name IN LISTS codec_error_tests)
        add_test(NAME ${test_name} COMMAND test_codec_error ${test_name})
    endforeach()
endif()
