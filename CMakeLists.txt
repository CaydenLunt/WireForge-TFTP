cmake_minimum_required(VERSION 3.16)

project(wireforge-tftp LANGUAGES C)

# Apply strict warnings to C targets defined in this directory and below.
add_compile_options(
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>"
)

include(CTest)

if(BUILD_TESTING)
    add_executable(test_codec_ack
        tests/unit/test_codec_ack.c
        src/protocol/codec.c
    )
    target_include_directories(test_codec_ack PRIVATE include)
    target_compile_features(test_codec_ack PRIVATE c_std_99)
    add_test(NAME codec_ack COMMAND test_codec_ack)
endif()
