#include "wireforge/tftp/codec.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

int main(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    uint8_t data[517] = {0, 3, 0xab, 0xcd};
    struct codec_ack_packet ack_packet = {ack, sizeof(ack), 0xbeef};
    struct codec_data_packet packet = {0};
    CHECK(!populate_ack_packet(NULL, ack, sizeof(ack)));
    CHECK(!populate_data_packet(NULL, data, 4));
    CHECK(!decode_data(NULL));
    CHECK(populate_ack_packet(&ack_packet, ack, sizeof(ack)));
    CHECK(ack_packet.buffer == ack && ack_packet.length == sizeof(ack) && ack_packet.block_number == 0);
    CHECK(decode_ack(&ack_packet) && ack_packet.block_number == 0x1234);
    CHECK(ack_packet.buffer == ack && ack_packet.length == sizeof(ack));

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
        CHECK(populate_data_packet(&packet, data, length) == valid);
        CHECK(packet.block_number == (valid ? 0 : 0xbeef));
        CHECK(packet.payload == (valid ? NULL : ack));
        CHECK(packet.payload_length == (valid ? 0 : 99));
    }

    const uint8_t bad[][4] = {{0, 4, 0, 1}, {1, 3, 0, 1}, {0, 0, 0, 1}};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        packet = (struct codec_data_packet){bad[i], 4, 0xbeef, ack, 99};
        CHECK(!decode_data(&packet) && packet.block_number == 0xbeef);
        CHECK(packet.payload == ack && packet.payload_length == 99);
    }
    packet = (struct codec_data_packet){NULL, 4, 0xbeef, ack, 99};
    CHECK(!decode_data(&packet) && packet.block_number == 0xbeef);
    CHECK(packet.payload == ack && packet.payload_length == 99);

    packet = (struct codec_data_packet){ack, sizeof(ack), 0xbeef, ack, 99};
    ack_packet.block_number = 0xbeef;
    CHECK(!populate_ack_packet(&ack_packet, data, 4));
    CHECK(!populate_ack_packet(&ack_packet, ack, 3));
    CHECK(!populate_ack_packet(&ack_packet, NULL, 4));
    CHECK(ack_packet.buffer == ack && ack_packet.length == sizeof(ack) && ack_packet.block_number == 0xbeef);
    CHECK(!populate_data_packet(&packet, ack, 4));
    CHECK(!populate_data_packet(&packet, data, 3));
    CHECK(!populate_data_packet(&packet, data, 517));
    CHECK(!populate_data_packet(&packet, NULL, 4));
    CHECK(packet.buffer == ack && packet.length == sizeof(ack) && packet.block_number == 0xbeef);
    CHECK(packet.payload == ack && packet.payload_length == 99);
    CHECK(populate_data_packet(&packet, data, 516));
    CHECK(packet.payload == NULL && packet.payload_length == 0);
    CHECK(packet.buffer == data && packet.length == 516 && packet.block_number == 0);
    CHECK(decode_data(&packet) && packet.block_number == 0xabcd);
    CHECK(data[2] == 0xab && data[3] == 0xcd);
    CHECK(packet.payload == data + 4 && packet.payload_length == 512);
    const uint8_t empty[] = {0, 3, 0, 1};
    packet = (struct codec_data_packet){empty, sizeof(empty), 0, NULL, 0};
    CHECK(decode_data(&packet));
    CHECK(packet.payload == empty + sizeof(empty) && packet.payload_length == 0);
    return 0;
}
