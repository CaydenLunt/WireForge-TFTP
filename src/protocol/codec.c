// #############################################################################
// # File: codec.c                                                             #
// # Description: This file contains the implementation of the encoder/decoder #
// #              Functions for the protocol.                                  #
// #                                                                           #
// #############################################################################

#include "wireforge/tftp/codec.h"

static bool codec_ack_buffer_is_valid(const struct codec_ack_packet *packet);
static bool codec_data_packet_header_is_valid(const uint8_t *buffer, size_t length);
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
static const size_t TFTP_DATA_PACKET_HEADER_LENGTH = 4;

/* Return the packet opcode in host order; requires two readable bytes. */
static uint16_t get_opcode(const uint8_t *buffer) {
    return (uint16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);
}

/* Return the ACK/DATA block number in host order.
 * Caller must supply a validated ACK or DATA buffer with four readable bytes.
 */
static uint16_t get_block_number(const uint8_t *buffer) {
    return (uint16_t)(((uint16_t)buffer[2] << 8) | buffer[3]);
}

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
 *   not guaranteed by sizeof(struct codec_ack_packet).
 *
 * @param packet Buffer, length, and decoded block number.
 *
 * @return true on success, false on failure.
 */
bool decode_ack(struct codec_ack_packet *packet) {
    if (!codec_ack_buffer_is_valid(packet)) {
        return false;
    }
    packet->block_number = get_block_number(packet->buffer);
    return true;
}

bool populate_ack_packet(struct codec_ack_packet *packet,
                         const uint8_t *buffer, size_t length) {
    const struct codec_ack_packet candidate = {buffer, length, 0};
    if (packet == NULL || !codec_ack_buffer_is_valid(&candidate)) {
        return false;
    }
    *packet = candidate;
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
 * @param packet Packet with length readable buffer bytes, or NULL.
 *
 * @return true if the buffer is non-NULL, its length is 4, and its opcode
 *         is 4; false otherwise.
 */
static bool codec_ack_buffer_is_valid(const struct codec_ack_packet *packet) {
    // Validate pointers and exact wire length before reading (ACK-SR-001/002/003).
    if (packet == NULL || packet->buffer == NULL ||
        packet->length != TFTP_ACK_PACKET_LENGTH) {
        return false;
    }
    return get_opcode(packet->buffer) == TFTP_OPCODE_ACK;
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
 * @param packet Buffer, length, and decoded block number.
 *
 * @return true on success, false on failure.
 */
bool decode_data(struct codec_data_packet *packet) {
    if (packet == NULL ||
        !codec_data_packet_header_is_valid(packet->buffer, packet->length)) {
        return false;
    }
    packet->block_number = get_block_number(packet->buffer);
    packet->payload = packet->buffer + TFTP_DATA_PACKET_HEADER_LENGTH;
    packet->payload_length = packet->length - TFTP_DATA_PACKET_HEADER_LENGTH;
    return true;
}

bool populate_data_packet(struct codec_data_packet *packet,
                          const uint8_t *buffer, size_t length) {
    if (packet == NULL || !codec_data_packet_header_is_valid(buffer, length)) {
        return false;
    }
    const struct codec_data_packet candidate = {buffer, length, 0, NULL, 0};
    *packet = candidate;
    return true;
}

static bool codec_data_packet_header_is_valid(const uint8_t *buffer, size_t length) {
    // Require a complete DATA packet before reading (DATA-FR-002, DATA-SR-003).
    if (buffer == NULL || length < TFTP_DATA_PACKET_HEADER_LENGTH) {
        return false;
    }

    // Reject an opcode other than 3 (DATA-FR-004).
    if (get_opcode(buffer) != TFTP_OPCODE_DATA) {
        return false;
    }

    return codec_data_payload_is_valid(buffer + TFTP_DATA_PACKET_HEADER_LENGTH, length - TFTP_DATA_PACKET_HEADER_LENGTH);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma region "ERROR Decoder Implementation"

bool decode_error(struct codec_error_packet *packet) {
    // ERROR decoding is not implemented yet; leave the packet unchanged.
    (void)packet;
    return false;
}

#pragma endregion "ERROR Decoder Implementation"
