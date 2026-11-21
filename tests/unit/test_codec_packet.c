#include "wireforge/tftp/codec.h"
#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

int main(void) {
    const uint8_t ack[] = {0, 4, 0x12, 0x34};
    uint8_t data[517] = {0, 3, 0xab, 0xcd};
    struct codec_packet packet = {ack, sizeof(ack), 0xbeef};
    CHECK(!populate_ack_packet(NULL, ack, sizeof(ack)));
    CHECK(!populate_data_packet(NULL, data, 4));
    CHECK(!decode_data(NULL));
    CHECK(populate_ack_packet(&packet, ack, sizeof(ack)));
    CHECK(packet.buffer == ack && packet.length == sizeof(ack) && packet.block_number == 0);
    CHECK(decode_ack(&packet) && packet.block_number == 0x1234);
    CHECK(packet.buffer == ack && packet.length == sizeof(ack));

    for (size_t length = 0; length <= sizeof(data); ++length) {
        packet.buffer = data;
        packet.length = length;
        packet.block_number = 0xbeef;
        const bool valid = length >= 4 && length <= 516;
        CHECK(decode_data(&packet) == valid);
        CHECK(packet.block_number == (valid ? 0xabcd : 0xbeef));
        CHECK(packet.buffer == data && packet.length == length);
        CHECK(populate_data_packet(&packet, data, length) == valid);
        CHECK(packet.block_number == (valid ? 0 : 0xbeef));
    }

    const uint8_t bad[][4] = {{0, 4, 0, 1}, {1, 3, 0, 1}, {0, 0, 0, 1}};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        packet = (struct codec_packet){bad[i], 4, 0xbeef};
        CHECK(!decode_data(&packet) && packet.block_number == 0xbeef);
    }
    packet = (struct codec_packet){NULL, 4, 0xbeef};
    CHECK(!decode_data(&packet) && packet.block_number == 0xbeef);

    packet = (struct codec_packet){ack, sizeof(ack), 0xbeef};
    CHECK(!populate_ack_packet(&packet, data, 4));
    CHECK(!populate_ack_packet(&packet, ack, 3));
    CHECK(!populate_ack_packet(&packet, NULL, 4));
    CHECK(!populate_data_packet(&packet, ack, 4));
    CHECK(!populate_data_packet(&packet, data, 3));
    CHECK(!populate_data_packet(&packet, data, 517));
    CHECK(!populate_data_packet(&packet, NULL, 4));
    CHECK(packet.buffer == ack && packet.length == sizeof(ack) && packet.block_number == 0xbeef);
    CHECK(populate_data_packet(&packet, data, 516));
    CHECK(packet.buffer == data && packet.length == 516 && packet.block_number == 0);
    CHECK(decode_data(&packet) && packet.block_number == 0xabcd);
    CHECK(data[2] == 0xab && data[3] == 0xcd);
    return 0;
}
