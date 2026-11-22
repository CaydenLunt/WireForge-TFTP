#ifndef WIREFORGE_TFTP_CODEC_HELPERS_H
#define WIREFORGE_TFTP_CODEC_HELPERS_H

#include <stdbool.h>
#include "utils/byte_order.h"

/* Return the packet opcode in host order; requires two readable bytes. */
static inline uint16_t get_opcode(const uint8_t *buffer) {
    return read_u16_be(buffer);
}

/* Check the pointer and length before reading the opcode. */
static inline bool has_opcode(const uint8_t *buffer, size_t length,
                              uint16_t expected_opcode) {
    return buffer != NULL && length >= 2 &&
           get_opcode(buffer) == expected_opcode;
}

/* Requires a validated ACK or DATA buffer with four readable bytes. */
static inline uint16_t get_block_number(const uint8_t *buffer) {
    return read_u16_be(buffer + 2);
}

/* Requires a validated ERROR packet with at least five readable bytes. */
static inline uint16_t get_error_code(const uint8_t *buffer) {
    return read_u16_be(buffer + 2);
}

#endif /* WIREFORGE_TFTP_CODEC_HELPERS_H */
