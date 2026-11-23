#ifndef WIREFORGE_TFTP_CODEC_HELPERS_H
#define WIREFORGE_TFTP_CODEC_HELPERS_H

#include <stdbool.h>
#include <stddef.h>
#include "utils/byte_order.h"

/* Validate a packet's inclusive length range and opcode before field reads.
 * Use SIZE_MAX for an unbounded maximum. Never modifies the input.
 */
bool codec_buffer_is_valid(const uint8_t *buffer, size_t length,
                           uint16_t opcode, size_t minimum, size_t maximum);

/* Find the first NUL at or after offset, bounded by length readable bytes.
 * When require_final is true, that NUL must be the final packet byte.
 * Returns NULL for invalid pointers/bounds or a missing/misplaced terminator.
 * Empty strings are allowed; callers enforce packet-specific restrictions.
 */
const uint8_t *codec_string_end(const uint8_t *buffer, size_t length,
                                size_t offset, bool require_final);

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
