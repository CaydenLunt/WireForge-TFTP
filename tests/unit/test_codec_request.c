#include "wireforge/tftp/codec.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #expr); return 1; } } while (0)

static int check_request(uint8_t opcode) {
    bool (*decode)(struct codec_request_block *) = opcode == 1 ? decode_rrq : decode_wrq;
    bool (*populate)(struct codec_request_block *, const uint8_t *, size_t) =
        opcode == 1 ? populate_rrq_packet : populate_wrq_packet;
    uint8_t valid[] = {0, opcode, 'f', 0, 'O', 'c', 'T', 'e', 'T', 0};
    uint8_t original[sizeof(valid)];
    memcpy(original, valid, sizeof(valid));
    struct codec_request_block packet = {0};
    CHECK(!decode(NULL));
    CHECK(!populate(NULL, valid, sizeof(valid)));
    CHECK(populate(&packet, valid, sizeof(valid)));
    CHECK(packet.buffer == valid && packet.length == sizeof(valid));
    CHECK(packet.opcode == 0 && packet.filename == NULL && packet.filename_length == 0);
    CHECK(packet.mode == NULL && packet.mode_length == 0);
    CHECK(decode(&packet));
    CHECK(packet.opcode == opcode && packet.filename == valid + 2 && packet.filename_length == 1);
    CHECK(packet.mode == valid + 4 && packet.mode_length == 5);
    CHECK(memcmp(original, valid, sizeof(valid)) == 0);

    const uint8_t malformed[][10] = {
        {1, opcode, 'f', 0, 'o', 0}, /* high opcode byte */
        {0, 3, 'f', 0, 'o', 0},
        {0, opcode == 1 ? 2 : 1, 'f', 0, 'o', 0},
        {0, opcode, 0, 'o', 'o', 0}, /* empty filename */
        {0, opcode, 'f', 'f', 'f', 'f'}, /* missing filename NUL */
        {0, opcode, 'f', 'f', 'f', 0}, /* no mode */
        {0, opcode, 'f', 'f', 0, 0}, /* empty mode */
        {0, opcode, 'f', 0, 'o', 'o'}, /* missing mode NUL */
        {0, opcode, 'f', 0, 'o', 0, 'x'}, /* trailing bytes */
        {0, opcode, 'f', 0, 'o', 0, 'x', 0, 'y', 0} /* options */
    };
    for (size_t i = 0; i < 2 + sizeof(valid) + sizeof(malformed) / sizeof(malformed[0]); ++i) {
        const uint8_t *buffer;
        size_t length;
        if (i < 2) {
            buffer = NULL;
            length = i == 0 ? 0 : sizeof(valid);
        } else if (i < 2 + sizeof(valid)) {
            buffer = valid;
            length = i - 2; /* every truncated prefix */
        } else {
            size_t index = i - 2 - sizeof(valid);
            buffer = malformed[index];
            length = index < 8 ? 6 : (index == 8 ? 7 : 10);
        }
        struct codec_request_block rejected = {buffer, length, 99, valid, 42, valid, 43};
        unsigned char saved[sizeof(rejected)];
        memcpy(saved, &rejected, sizeof(rejected));
        CHECK(!decode(&rejected));
        CHECK(memcmp(saved, &rejected, sizeof(rejected)) == 0);
        CHECK(!populate(&rejected, buffer, length));
        CHECK(memcmp(saved, &rejected, sizeof(rejected)) == 0);
    }
    return 0;
}

int main(void) {
    CHECK(check_request(1) == 0);
    CHECK(check_request(2) == 0);
    return 0;
}
