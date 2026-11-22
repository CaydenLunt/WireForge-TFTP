#include "wireforge/tftp/codec.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

static int test_populate_ack_packet_rejects_null_output(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    CHECK(!populate_ack_packet(NULL, ack, sizeof(ack)));
    return 0;
}

static int test_populate_data_packet_rejects_null_output(void) {
    const uint8_t data[] = {0, 3, 0xab, 0xcd};
    CHECK(!populate_data_packet(NULL, data, sizeof(data)));
    return 0;
}

static int test_decode_data_rejects_null_output(void) {
    CHECK(!decode_data(NULL));
    return 0;
}

static int test_populate_ack_packet_initializes_fields(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    struct codec_ack_packet ack_packet = {ack, sizeof(ack), 0xbeef};
    CHECK(populate_ack_packet(&ack_packet, ack, sizeof(ack)));
    CHECK(ack_packet.buffer == ack && ack_packet.length == sizeof(ack) && ack_packet.block_number == 0);
    CHECK(decode_ack(&ack_packet) && ack_packet.block_number == 0x1234);
    CHECK(ack_packet.buffer == ack && ack_packet.length == sizeof(ack));
    return 0;
}

static int test_decode_data_validates_lengths_and_preserves_payload(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    uint8_t data[517] = {0, 3, 0xab, 0xcd};
    struct codec_data_packet packet = {0};
    for (size_t i = 4; i < sizeof(data); ++i) {
        data[i] = (uint8_t)(i & 0xff);
    }
    uint8_t original[sizeof(data)];
    memcpy(original, data, sizeof(data));
    for (size_t length = 0; length <= sizeof(data); ++length) {
        packet.buffer = data;
        packet.length = length;
        packet.block_number = 0xbeef;
        packet.payload = ack;
        packet.payload_length = 99;
        const bool valid = length >= 4 && length <= 516;
        CHECK(decode_data(&packet) == valid);
        CHECK(packet.block_number == (valid ? 0xabcd : 0xbeef));
        CHECK(packet.buffer == data && packet.length == length);
        CHECK(packet.payload == (valid ? data + 4 : ack));
        CHECK(packet.payload_length == (valid ? length - 4 : 99));
        if (valid) {
            CHECK(memcmp(packet.payload, original + 4, packet.payload_length) == 0);
        }
        CHECK(memcmp(data, original, sizeof(data)) == 0);
    }
    return 0;
}

static int test_populate_data_packet_validates_lengths(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    const uint8_t data[517] = {0, 3, 0xab, 0xcd};
    for (size_t length = 0; length <= sizeof(data); ++length) {
        struct codec_data_packet packet = {data, length, 0xbeef, ack, 99};
        const bool valid = length >= 4 && length <= 516;
        CHECK(populate_data_packet(&packet, data, length) == valid);
        CHECK(packet.buffer == data && packet.length == length);
        CHECK(packet.block_number == (valid ? 0 : 0xbeef));
        CHECK(packet.payload == (valid ? NULL : ack));
        CHECK(packet.payload_length == (valid ? 0 : 99));
    }
    return 0;
}

static int test_decode_data_rejects_invalid_opcode(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    const uint8_t bad[][4] = {{0, 4, 0, 1}, {1, 3, 0, 1}, {0, 0, 0, 1}};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        struct codec_data_packet packet = {bad[i], 4, 0xbeef, ack, 99};
        CHECK(!decode_data(&packet) && packet.block_number == 0xbeef);
        CHECK(packet.payload == ack && packet.payload_length == 99);
    }
    return 0;
}

static int test_decode_data_rejects_null_input(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    struct codec_data_packet packet = {NULL, 4, 0xbeef, ack, 99};
    CHECK(!decode_data(&packet) && packet.block_number == 0xbeef);
    CHECK(packet.payload == ack && packet.payload_length == 99);
    return 0;
}

static int test_populate_ack_packet_preserves_output_on_failure(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    const uint8_t data[] = {0, 3, 0xab, 0xcd};
    struct codec_ack_packet ack_packet = {ack, sizeof(ack), 0xbeef};
    CHECK(!populate_ack_packet(&ack_packet, data, 4));
    CHECK(!populate_ack_packet(&ack_packet, ack, 3));
    CHECK(!populate_ack_packet(&ack_packet, NULL, 4));
    CHECK(ack_packet.buffer == ack && ack_packet.length == sizeof(ack) && ack_packet.block_number == 0xbeef);
    return 0;
}

static int test_populate_data_packet_preserves_output_on_failure(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    const uint8_t data[517] = {0, 3, 0xab, 0xcd};
    struct codec_data_packet packet = {ack, sizeof(ack), 0xbeef, ack, 99};
    CHECK(!populate_data_packet(&packet, ack, 4));
    CHECK(!populate_data_packet(&packet, data, 3));
    CHECK(!populate_data_packet(&packet, data, 517));
    CHECK(!populate_data_packet(&packet, NULL, 4));
    CHECK(packet.buffer == ack && packet.length == sizeof(ack) && packet.block_number == 0xbeef);
    CHECK(packet.payload == ack && packet.payload_length == 99);
    return 0;
}

static int test_populate_data_packet_initializes_fields(void) {
    const uint8_t data[516] = {0, 3, 0xab, 0xcd};
    struct codec_data_packet packet = {0};
    CHECK(populate_data_packet(&packet, data, 516));
    CHECK(packet.payload == NULL && packet.payload_length == 0);
    CHECK(packet.buffer == data && packet.length == 516 && packet.block_number == 0);
    CHECK(decode_data(&packet) && packet.block_number == 0xabcd);
    CHECK(data[2] == 0xab && data[3] == 0xcd);
    CHECK(packet.payload == data + 4 && packet.payload_length == 512);
    return 0;
}

static int test_decode_data_accepts_empty_payload(void) {
    const uint8_t empty[] = {0, 3, 0, 1};
    struct codec_data_packet packet = {empty, sizeof(empty), 0, NULL, 0};
    CHECK(decode_data(&packet));
    CHECK(packet.payload == empty + sizeof(empty) && packet.payload_length == 0);
    return 0;
}

int main(int argc, char *argv[]) {
    const struct {
        const char *name;
        int (*run)(void);
    } tests[] = {
#define TEST(name) {#name, test_##name}
        TEST(populate_ack_packet_rejects_null_output),
        TEST(populate_data_packet_rejects_null_output),
        TEST(decode_data_rejects_null_output),
        TEST(populate_ack_packet_initializes_fields),
        TEST(decode_data_validates_lengths_and_preserves_payload),
        TEST(populate_data_packet_validates_lengths),
        TEST(decode_data_rejects_invalid_opcode),
        TEST(decode_data_rejects_null_input),
        TEST(populate_ack_packet_preserves_output_on_failure),
        TEST(populate_data_packet_preserves_output_on_failure),
        TEST(populate_data_packet_initializes_fields),
        TEST(decode_data_accepts_empty_payload)
#undef TEST
    };
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <test_name>\n", argv[0]);
        return 1;
    }
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        if (strcmp(argv[1], tests[i].name) == 0) {
            return tests[i].run();
        }
    }
    fprintf(stderr, "Unknown test: %s\n", argv[1]);
    return 1;
}
