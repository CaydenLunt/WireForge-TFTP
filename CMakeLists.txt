cmake_minimum_required(VERSION 3.16)

project(wireforge-tftp LANGUAGES C)

# Apply strict warnings to C targets defined in this directory and below.
add_compile_options(
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>"
)

include(CTest)

if(BUILD_TESTING)
    add_executable(test_codec_packet tests/unit/test_codec_packet.c src/protocol/codec.c)
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
        src/protocol/codec.c
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
endif()
