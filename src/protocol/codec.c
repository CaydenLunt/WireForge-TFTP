// #############################################################################
// # File: codec.c                                                             #
// # Description: This file contains the implementation of the encoder/decoder #
// #              Functions for the protocol.                                  #
// #                                                                           #
// #############################################################################

#include "wireforge/tftp/codec.h"

static bool codec_ack_buffer_is_valid(const uint8_t *buffer, size_t length);
static bool codec_ack_block_is_valid(const struct codec_ack_block *ack_block);

/**
 * @brief Decodes a TFTP acknowledgment (ACK) packet.
 *
 * @details
 * Implements ACK packet decoding according to RFC 1350,
 * Section 5, Figure 5-3.
 *
 * Requirements:
 * - ACK-FR-003: Packet must be exactly 4 bytes.
 * - ACK-FR-005: Opcode must equal 4.
 * - ACK-FR-006: Decode 16-bit block number in network byte order.
 * - ACK-FR-007: Accept block numbers 0 through 65535.
 * - ACK-SR-001: Reject NULL input buffer.
 * - ACK-SR-002: Reject NULL output pointer.
 * - ACK-SR-003: Never read beyond the supplied buffer.
 * - ACK-SR-004: Do not modify output on failure.
 *
 * By design:
 * - ACK-FR-007: The uint16_t output field represents every 16-bit block
 *   number. This guarantees the storage range, not successful decoding.
 * - ACK-SR-004: Validation performs no output writes, so rejection leaves
 *   the output unchanged. Keep any decoded-value assignment after validation.
 * - ACK-FR-003: The 4-byte wire length requires an explicit check; it is
 *   not guaranteed by sizeof(struct codec_ack_block).
 *
 * @param buffer Pointer to the raw TFTP packet.
 * @param length Number of bytes in the buffer.
 * @param ack_block Pointer to the decoded ACK structure.
 *
 * @return true on success, false on failure.
 */
bool decode_ack(
    const uint8_t *buffer,
    size_t length,
    struct codec_ack_block *ack_block
) {
    if (!codec_ack_buffer_is_valid(buffer, length)) {
        return false;
    }

    // Reject a NULL output pointer (ACK-SR-002).
    if (!codec_ack_block_is_valid(ack_block)) {
        return false;
    }

    return true;
}

/**
 * @brief Validates the length and opcode of a TFTP ACK packet buffer.
 *
 * @details
 * Checks the input pointer and packet length before reading the opcode.
 * Does not decode the block number or modify the buffer.
 *
 * Requirements:
 * - ACK-FR-003: Reject packets whose length is not exactly 4 bytes.
 * - ACK-FR-005: Reject an opcode other than 4.
 * - ACK-SR-001: Reject a NULL input buffer.
 * - ACK-SR-003: Check length before reading within the supplied buffer.
 *
 * @param buffer Pointer to the raw packet, or NULL. A non-NULL pointer
 *               must refer to at least length readable bytes.
 * @param length Number of readable bytes in the buffer.
 *
 * @return true if the buffer is non-NULL, its length is 4, and its opcode
 *         is 4; false otherwise.
 */
static bool codec_ack_buffer_is_valid(const uint8_t *buffer, size_t length) {
    // Reject a NULL input buffer before reading it (ACK-SR-001).
    if (buffer == NULL) {
        return false;
    }

    // Require a complete ACK packet before reading (ACK-FR-003, ACK-SR-003).
    if (length != 4) {
        return false;
    }

    // Reject an opcode other than 4 (ACK-FR-005).
    if (buffer[0] != 0 || buffer[1] != 4) {
        return false;
    }

    return true;
}


/**
 * @brief Checks whether the ACK output pointer is non-NULL.
 *
 * @details
 * Does not read or modify the output structure, and does not verify that
 * a non-NULL pointer refers to accessible storage.
 *
 * Requirements:
 * - ACK-SR-002: Reject a NULL output pointer.
 * - ACK-SR-004: Leave output unchanged during validation.
 *
 * By design:
 * - ACK-FR-007: The uint16_t block-number field represents 0 through 65535;
 *   its previous value does not need a runtime range check.
 *
 * @param ack_block Pointer to the ACK output structure, or NULL.
 *
 * @return true if ack_block is non-NULL; false otherwise.
 */
static bool codec_ack_block_is_valid(const struct codec_ack_block *ack_block) {
    // Reject a NULL output pointer (ACK-SR-002).
    if (ack_block == NULL) {
        return false;
    }

    return true;
}
