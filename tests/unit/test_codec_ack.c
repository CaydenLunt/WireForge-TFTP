#include "wireforge/tftp/codec.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Verifies that valid ACK packets decode in network byte order.
 * @details Covers single-byte values, the 255-to-256 transition, reversed
 *          byte patterns, and values around the signed 16-bit boundary.
 *          Each case starts with an output value different from its expected result.
 * @returns true if every packet decodes to its expected block; false otherwise.
 */
static bool test_decode_ack_reads_block_in_network_byte_order(void) {
    const struct {
        uint8_t packet[4];
        uint16_t expected_block_number;
    } cases[] = {
        {{0x00, 0x04, 0x00, 0x01}, 1},
        {{0x00, 0x04, 0x00, 0xFF}, 255},
        {{0x00, 0x04, 0x01, 0x00}, 256},
        {{0x00, 0x04, 0x01, 0x01}, 257},
        {{0x00, 0x04, 0x12, 0x34}, 0x1234},
        {{0x00, 0x04, 0x34, 0x12}, 0x3412},
        {{0x00, 0x04, 0x7F, 0xFF}, 32767},
        {{0x00, 0x04, 0x80, 0x00}, 32768},
        {{0x00, 0x04, 0xFF, 0x00}, 65280},
        {{0x00, 0x04, 0xFF, 0xFE}, 65534}
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        struct codec_ack_block actual = {0xBEEF};
        const bool success = decode_ack(cases[i].packet, sizeof(cases[i].packet), &actual);
        if (!success || actual.block_number != cases[i].expected_block_number) {
            fprintf(stderr, "%s: expected successful decoding of block %u, got success=%d, block=%u\n",
                    __func__, (unsigned int)cases[i].expected_block_number,
                    (int)success, (unsigned int)actual.block_number);
            return false;
        }
    }

    return true;
}

/**
 * @brief Verifies that the ACK decoder rejects a DATA opcode.
 * @details Supplies a four-byte packet with opcode 3 instead of ACK opcode 4.
 * @returns true if decoding rejects the packet; false otherwise.
 */
static bool test_decode_ack_rejects_invalid_opcode(void) {
    const uint8_t packet[] = {0x00, 0x03, 0x12, 0x34}; // opcode 3 (DATA)
    struct codec_ack_block actual = {0};

    const bool success = decode_ack(packet, sizeof(packet), &actual);

    if (success) {
        fprintf(stderr, "%s: expected decoding to fail for invalid opcode\n",
                __func__);
        return false;
    }

    return true;
}

/**
 * @brief Verifies that the ACK decoder rejects a truncated packet.
 * @details Supplies a three-byte packet with a valid ACK opcode but an
 *          incomplete block number; an ACK packet requires four bytes.
 * @returns true if decoding rejects the packet; false otherwise.
 */
static bool test_decode_ack_rejects_invalid_length(void) {
    const uint8_t packet[] = {0x00, 0x04, 0x12}; // length 3 (invalid)
    struct codec_ack_block actual = {0};

    const bool success = decode_ack(packet, sizeof(packet), &actual);

    if (success) {
        fprintf(stderr, "%s: expected decoding to fail for invalid length\n",
                __func__);
        return false;
    }

    return true;
}

/**
 * @brief Verifies that the ACK decoder rejects a NULL output pointer.
 * @details Supplies a valid ACK packet without storage for the decoded result.
 * @returns true if decoding rejects the NULL output pointer; false otherwise.
 */
static bool test_decode_ack_rejects_null_output_pointer(void) {
    const uint8_t packet[] = {0x00, 0x04, 0x12, 0x34};

    const bool success = decode_ack(packet, sizeof(packet), NULL);

    if (success) {
        fprintf(stderr, "%s: expected decoding to fail for NULL output pointer\n",
                __func__);
        return false;
    }

    return true;
}

/**
 * @brief Verifies that a NULL input is rejected with a nonzero length.
 * @returns true if decoding fails; false otherwise.
 */
static bool test_decode_ack_rejects_null_input_pointer(void) {
    struct codec_ack_block actual = {0};

    const bool success = decode_ack(NULL, 4, &actual);

    if (success) {
        fprintf(stderr, "%s: expected decoding to fail for NULL input pointer\n",
                __func__);
        return false;
    }

    return true;
}

/**
 * @brief Verifies that a NULL input is rejected even with zero length.
 * @returns true if decoding fails; false otherwise.
 */
static bool test_decode_ack_rejects_null_input_pointer_with_zero_length(void) {
    struct codec_ack_block actual = {0};

    const bool success = decode_ack(NULL, 0, &actual);

    if (success) {
        fprintf(stderr, "%s: expected decoding to fail for NULL input pointer with zero length\n",
                __func__);
        return false;
    }

    return true;
}

/**
 * @brief Verifies that both boundary block numbers decode successfully.
 * @details Checks blocks 0 and 65535, starting with a different output value.
 * @returns true if both packets decode to their expected blocks; false otherwise.
 */
static bool test_decode_ack_accepts_boundary_block_numbers(void) {
    const uint8_t packets[][4] = {
        {0x00, 0x04, 0x00, 0x00},
        {0x00, 0x04, 0xFF, 0xFF}
    };
    const uint16_t expected[] = {0, UINT16_MAX};

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        struct codec_ack_block actual = {0xBEEF};
        const bool success = decode_ack(packets[i], sizeof(packets[i]), &actual);
        if (!success || actual.block_number != expected[i]) {
            fprintf(stderr, "%s: expected successful decoding of block %u, got success=%d, block=%u\n",
                    __func__, (unsigned int)expected[i], (int)success,
                    (unsigned int)actual.block_number);
            return false;
        }
    }

    return true;
}

/**
 * @brief Verifies rejection and unchanged output for malformed inputs.
 * @details Covers lengths 0 through 3 and 5, both opcode bytes, and NULL
 *          input with lengths 0 and 4. Each case starts with block 0xBEEF.
 * @returns true if every input is rejected without changing output; false otherwise.
 */
static bool test_decode_ack_preserves_output_on_failure(void) {
    const uint8_t empty_buffer[] = {0x00};
    const uint8_t one_byte[] = {0x00};
    const uint8_t two_bytes[] = {0x00, 0x04};
    const uint8_t three_bytes[] = {0x00, 0x04, 0x12};
    const uint8_t five_bytes[] = {0x00, 0x04, 0x12, 0x34, 0x00};
    const uint8_t invalid_opcode[] = {0x00, 0x03, 0x12, 0x34};
    const uint8_t invalid_opcode_high_byte[] = {0x01, 0x04, 0x12, 0x34};
    const struct {
        const char *name;
        const uint8_t *packet;
        size_t length;
    } cases[] = {
        {"zero length", empty_buffer, 0},
        {"one byte", one_byte, sizeof(one_byte)},
        {"two bytes", two_bytes, sizeof(two_bytes)},
        {"three bytes", three_bytes, sizeof(three_bytes)},
        {"five bytes", five_bytes, sizeof(five_bytes)},
        {"invalid opcode", invalid_opcode, sizeof(invalid_opcode)},
        {"nonzero opcode high byte", invalid_opcode_high_byte, sizeof(invalid_opcode_high_byte)},
        {"NULL input, zero length", NULL, 0},
        {"NULL input, length four", NULL, 4}
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        struct codec_ack_block actual = {0xBEEF};
        const bool success = decode_ack(cases[i].packet, cases[i].length, &actual);
        if (success || actual.block_number != 0xBEEF) {
            fprintf(stderr, "%s (%s): expected rejection with block unchanged at %u, got success=%d, block=%u\n",
                    __func__, cases[i].name, (unsigned int)0xBEEF,
                    (int)success, (unsigned int)actual.block_number);
            return false;
        }
    }

    return true;
}

/**
 * @brief Verifies rejection when both input and output pointers are NULL.
 * @details Uses the valid ACK length of four bytes to exercise simultaneous
 *          invalid pointers without also supplying an invalid length.
 * @returns true if decoding returns false without crashing; false otherwise.
 */
static bool test_decode_ack_rejects_both_null_pointers(void) {
    const bool success = decode_ack(NULL, 4, NULL);

    if (success) {
        fprintf(stderr, "%s: expected decoding to fail for two NULL pointers\n",
                __func__);
        return false;
    }

    return true;
}

int main(void) {
    if (!test_decode_ack_reads_block_in_network_byte_order()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_rejects_invalid_length()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_rejects_invalid_opcode()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_rejects_null_output_pointer()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_rejects_null_input_pointer()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_rejects_null_input_pointer_with_zero_length()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_rejects_both_null_pointers()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_accepts_boundary_block_numbers()) {
        return EXIT_FAILURE;
    }

    if (!test_decode_ack_preserves_output_on_failure()) {
        return EXIT_FAILURE;
    }

    puts("ACK codec tests passed");
    return EXIT_SUCCESS;
}
