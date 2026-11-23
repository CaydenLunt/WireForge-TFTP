#include "wireforge/tftp/codec.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #expr); return 1; } } while (0)

static int check_request(uint8_t opcode, const char *test_name) {
    bool (*decode)(struct codec_request_block *) = opcode == 1 ? decode_rrq : decode_wrq;
    bool (*populate)(struct codec_request_block *, const uint8_t *, size_t) =
        opcode == 1 ? populate_rrq_packet : populate_wrq_packet;
    uint8_t valid[] = {0, opcode, 'f', 0, 'O', 'c', 'T', 'e', 'T', 0};
    uint8_t original[sizeof(valid)];
    memcpy(original, valid, sizeof(valid));
    if (strcmp(test_name, "rejects_null_decode_output") == 0) {
        CHECK(!decode(NULL));
        return 0;
    }
    if (strcmp(test_name, "rejects_null_populate_output") == 0) {
        CHECK(!populate(NULL, valid, sizeof(valid)));
        return 0;
    }
    if (strcmp(test_name, "initializes_and_decodes_fields") == 0) {
        struct codec_request_block packet = {valid, sizeof(valid), 99, valid, 42, valid, 43};
        CHECK(populate(&packet, valid, sizeof(valid)));
        CHECK(packet.buffer == valid && packet.length == sizeof(valid));
        CHECK(packet.opcode == 0 && packet.filename == NULL && packet.filename_length == 0);
        CHECK(packet.mode == NULL && packet.mode_length == 0);
        CHECK(decode(&packet));
        CHECK(packet.opcode == opcode && packet.filename == valid + 2 && packet.filename_length == 1);
        CHECK(packet.mode == valid + 4 && packet.mode_length == 5);
        CHECK(memcmp(original, valid, sizeof(valid)) == 0);
        return 0;
    }

    const char *malformed_names[] = {
        "rejects_high_opcode_byte", "rejects_data_opcode", "rejects_other_request_opcode",
        "rejects_empty_filename", "rejects_missing_filename_terminator", "rejects_missing_mode",
        "rejects_empty_mode", "rejects_missing_mode_terminator", "rejects_trailing_bytes",
        "rejects_options"
    };
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
        const char *name;
        char prefix_name[48];
        if (i < 2) {
            buffer = NULL;
            length = i == 0 ? 0 : sizeof(valid);
            name = i == 0 ? "rejects_null_input_zero_length" : "rejects_null_input";
        } else if (i < 2 + sizeof(valid)) {
            buffer = valid;
            length = i - 2; /* every truncated prefix */
            snprintf(prefix_name, sizeof(prefix_name), "rejects_truncated_prefix_%zu", length);
            name = prefix_name;
        } else {
            size_t index = i - 2 - sizeof(valid);
            buffer = malformed[index];
            name = malformed_names[index];
            length = index < 8 ? 6 : (index == 8 ? 7 : 10);
        }
        if (strcmp(test_name, name) != 0) {
            continue;
        }
        struct codec_request_block rejected = {buffer, length, 99, valid, 42, valid, 43};
        unsigned char saved[sizeof(rejected)];
        memcpy(saved, &rejected, sizeof(rejected));
        CHECK(!decode(&rejected));
        CHECK(memcmp(saved, &rejected, sizeof(rejected)) == 0);
        CHECK(!populate(&rejected, buffer, length));
        CHECK(memcmp(saved, &rejected, sizeof(rejected)) == 0);
        return 0;
    }
    fprintf(stderr, "Unknown test: %s\n", test_name);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3 || (strcmp(argv[1], "rrq") != 0 && strcmp(argv[1], "wrq") != 0)) {
        fprintf(stderr, "Usage: %s <rrq|wrq> <test_name>\n", argv[0]);
        return 1;
    }
    return check_request(strcmp(argv[1], "rrq") == 0 ? 1 : 2, argv[2]);
}
