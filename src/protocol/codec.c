// #############################################################################
// # File: codec.c                                                             #
// # Description: This file contains the implementation of the encoder/decoder #
// #              Functions for the protocol.                                  #
// #                                                                           #
// #############################################################################

#include "wireforge/tftp/codec.h"

static bool codec_ack_buffer_is_valid(const uint8_t *buffer, size_t length);
static bool codec_data_payload_is_valid(const uint8_t *payload, size_t length);

// Opcodes
static const uint8_t TFTP_OPCODE_RRQ   = 1;
static const uint8_t TFTP_OPCODE_WRQ   = 2;
static const uint8_t TFTP_OPCODE_DATA  = 3;
static const uint8_t TFTP_OPCODE_ACK   = 4;
static const uint8_t TFTP_OPCODE_ERROR = 5;

// ack vals
static const uint8_t TFTP_ACK_PACKET_LENGTH = 4;

// data vals
static const size_t TFTP_DATA_PAYLOAD_MAX_LENGTH = 512;

#pragma region "ACK Decoder Implementation"

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
    if (ack_block == NULL) {
        return false;
    }

    // get block number from buffer[2] and buffer[3] in network byte order
    uint16_t block_number = (buffer[2] << 8) | buffer[3];

    // Assign the decoded block number to the output structure.
    ack_block->block_number = block_number;

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
    if (length != TFTP_ACK_PACKET_LENGTH) {
        return false;
    }

    // Reject an opcode other than 4 (ACK-FR-005).
    if (buffer[0] != 0 || buffer[1] != TFTP_OPCODE_ACK) {
        return false;
    }

    return true;
}

#pragma endregion "ACK Decoder Implementation"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma region "DATA Decoder Implementation"


/**
 * @brief Decodes a TFTP DATA packet.
 *
 * @details
 * Implements DATA packet decoding according to RFC 1350,
 * Section 5, Figure 5-2.
 *
 * Requirements:
 * - DATA-FR-002: Packet must be at least 4 bytes.
 * - DATA-FR-003: Standard packet must not exceed 516 bytes.
 * - DATA-FR-004: Opcode must equal 3.
 * - DATA-FR-005: Decode 16-bit block number in network byte order.
 * - DATA-FR-006: Accept block numbers 0 through 65535.
 * - DATA-FR-007: Payload begins at byte offset 4.
 * - DATA-FR-008: Payload length equals length minus 4.
 * - DATA-FR-009: Accept payload lengths 0 through 512.
 * - DATA-FR-010: Preserve payload bytes exactly.
 * - DATA-SR-001: Reject NULL input buffer.
 * - DATA-SR-002: Reject NULL output pointer.
 * - DATA-SR-003: Never read beyond the supplied buffer.
 * - DATA-SR-004: Do not modify output on failure.
 * - DATA-SR-005: Do not modify input buffer.
 * - DATA-SR-006: Keep payload within input buffer bounds.
 *
 * @param buffer Pointer to the raw TFTP DATA packet.
 * @param length Number of readable bytes in the buffer.
 * @param data_block Pointer to the decoded DATA structure.
 *
 * @return true on success, false on failure.
 */
bool decode_data(
    const uint8_t *buffer,
    size_t length,
    struct codec_data_block *data_block
) {

}

/**
 * @brief Validates a DATA payload pointer and length, excluding the packet header.
 * @param payload Non-NULL pointer to at least length readable payload bytes.
 * @param length Payload length in bytes; zero is valid.
 * @return true for a non-NULL payload with length 0 through 512; false otherwise.
 */
static bool codec_data_payload_is_valid(const uint8_t *payload, size_t length) {
    if (payload == NULL) {
        return false;
    }

    // size_t is unsigned, so only the upper bound needs checking (DATA-FR-009).
    if (length > TFTP_DATA_PAYLOAD_MAX_LENGTH) {
        return false;
    }

    return true;
}

#pragma endregion "DATA Decoder Implementation"
