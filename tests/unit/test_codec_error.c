#include "wireforge/tftp/codec.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #expr); return 1; } } while (0)

/* Check both public entry points, including every field after rejection. */
static int rejects_unchanged(const uint8_t *buffer, size_t length) {
    const char sentinel[] = "old";
    struct codec_error_packet packet = {buffer, length, 0xbeef, sentinel, 3};
    CHECK(!decode_error(&packet));
    CHECK(packet.buffer == buffer && packet.length == length);
    CHECK(packet.error_code == 0xbeef && packet.error_message == sentinel && packet.error_message_length == 3);
    CHECK(!populate_error_packet(&packet, buffer, length));
    CHECK(packet.buffer == buffer && packet.length == length);
    CHECK(packet.error_code == 0xbeef && packet.error_message == sentinel && packet.error_message_length == 3);
    return 0;
}

static int test_decode_error_reads_code_and_message(void) {
    uint8_t buffer[] = {0, 5, 0x12, 0x34, 'B', 'a', 'd', 0};
    uint8_t original[sizeof(buffer)];
    memcpy(original, buffer, sizeof(buffer));
    struct codec_error_packet packet = {buffer, sizeof(buffer), 0, NULL, 0};
    CHECK(decode_error(&packet));
    CHECK(packet.error_code == 0x1234);
    CHECK(packet.error_message == (const char *)buffer + 4);
    CHECK(packet.error_message_length == 3);
    CHECK(strcmp(packet.error_message, "Bad") == 0);
    CHECK(packet.buffer == buffer && packet.length == sizeof(buffer));
    CHECK(memcmp(buffer, original, sizeof(buffer)) == 0);
    return 0;
}

static int test_decode_error_accepts_empty_message(void) {
    const uint8_t buffer[] = {0, 5, 0, 1, 0};
    struct codec_error_packet packet = {buffer, sizeof(buffer), 0, NULL, 99};
    CHECK(decode_error(&packet));
    CHECK(packet.error_code == 1 && packet.error_message_length == 0);
    CHECK(packet.error_message == (const char *)buffer + 4 && *packet.error_message == 0);
    return 0;
}

static int test_decode_error_accepts_known_and_unknown_codes(void) {
    const uint16_t codes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 255, 256, 32768, 65535};
    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); ++i) {
        const uint8_t buffer[] = {0, 5, (uint8_t)(codes[i] >> 8), (uint8_t)codes[i], 0};
        struct codec_error_packet packet = {buffer, sizeof(buffer), 42, NULL, 0};
        CHECK(decode_error(&packet) && packet.error_code == codes[i]);
    }
    return 0;
}

static int test_decode_error_accepts_long_message(void) {
    uint8_t buffer[1029];
    memset(buffer, 'x', sizeof(buffer));
    buffer[0] = 0; buffer[1] = 5; buffer[2] = 0; buffer[3] = 2;
    buffer[sizeof(buffer) - 1] = 0;
    struct codec_error_packet packet = {buffer, sizeof(buffer), 0, NULL, 0};
    CHECK(decode_error(&packet));
    CHECK(packet.error_message_length == 1024);
    CHECK(memcmp(packet.error_message, buffer + 4, 1025) == 0);
    return 0;
}

static int test_decode_error_rejects_null_output(void) {
    CHECK(!decode_error(NULL));
    return 0;
}

static int test_populate_error_packet_rejects_null_output(void) {
    const uint8_t buffer[] = {0, 5, 0, 1, 0};
    CHECK(!populate_error_packet(NULL, buffer, sizeof(buffer)));
    return 0;
}

static int test_error_packet_rejects_null_input(void) {
    CHECK(rejects_unchanged(NULL, 0) == 0);
    CHECK(rejects_unchanged(NULL, 5) == 0);
    return 0;
}

static int test_error_packet_rejects_truncated_header(void) {
    const uint8_t buffer[] = {0, 5, 0, 1, 0};
    for (size_t length = 0; length < 5; ++length) {
        CHECK(rejects_unchanged(buffer, length) == 0);
    }
    return 0;
}

static int test_error_packet_rejects_invalid_opcode(void) {
    const uint8_t buffers[][5] = {{0, 4, 0, 1, 0}, {1, 5, 0, 1, 0}, {0, 0, 0, 1, 0}};
    for (size_t i = 0; i < sizeof(buffers) / sizeof(buffers[0]); ++i) {
        CHECK(rejects_unchanged(buffers[i], sizeof(buffers[i])) == 0);
    }
    return 0;
}

static int test_error_packet_rejects_missing_terminator(void) {
    const uint8_t buffer[] = {0, 5, 0, 1, 'B', 'a', 'd', 0};
    CHECK(rejects_unchanged(buffer, sizeof(buffer) - 1) == 0);
    return 0;
}

static int test_error_packet_rejects_trailing_bytes(void) {
    const uint8_t buffer[] = {0, 5, 0, 1, 'x', 0, 'y', 0};
    CHECK(rejects_unchanged(buffer, sizeof(buffer)) == 0);
    CHECK(rejects_unchanged(buffer, sizeof(buffer) - 1) == 0);
    return 0;
}

static int test_populate_error_packet_initializes_fields(void) {
    const uint8_t buffer[] = {0, 5, 0, 7, 'x', 0};
    struct codec_error_packet packet = {NULL, 99, 42, "old", 3};
    CHECK(populate_error_packet(&packet, buffer, sizeof(buffer)));
    CHECK(packet.buffer == buffer && packet.length == sizeof(buffer));
    CHECK(packet.error_code == 0 && packet.error_message == NULL && packet.error_message_length == 0);
    CHECK(decode_error(&packet));
    CHECK(packet.error_code == 7 && packet.error_message_length == 1);
    return 0;
}

int main(int argc, char *argv[]) {
    const struct { const char *name; int (*run)(void); } tests[] = {
        {"decode_error_reads_code_and_message", test_decode_error_reads_code_and_message},
        {"decode_error_accepts_empty_message", test_decode_error_accepts_empty_message},
        {"decode_error_accepts_known_and_unknown_codes", test_decode_error_accepts_known_and_unknown_codes},
        {"decode_error_accepts_long_message", test_decode_error_accepts_long_message},
        {"decode_error_rejects_null_output", test_decode_error_rejects_null_output},
        {"populate_error_packet_rejects_null_output", test_populate_error_packet_rejects_null_output},
        {"error_packet_rejects_null_input", test_error_packet_rejects_null_input},
        {"error_packet_rejects_truncated_header", test_error_packet_rejects_truncated_header},
        {"error_packet_rejects_invalid_opcode", test_error_packet_rejects_invalid_opcode},
        {"error_packet_rejects_missing_terminator", test_error_packet_rejects_missing_terminator},
        {"error_packet_rejects_trailing_bytes", test_error_packet_rejects_trailing_bytes},
        {"populate_error_packet_initializes_fields", test_populate_error_packet_initializes_fields}
    };
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <test_name>\n", argv[0]);
        return 1;
    }
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        if (strcmp(argv[1], tests[i].name) == 0) return tests[i].run();
    }
    fprintf(stderr, "Unknown test: %s\n", argv[1]);
    return 1;
}
