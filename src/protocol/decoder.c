// #############################################################################
// # File: decoder.c                                                           #
// # Description: TFTP packet validation, decoding, and initialization.        #
// #                                                                           #
// #############################################################################

#include "wireforge/tftp/codec.h"
#include "utils/codec_helpers.h"

static bool codec_ack_buffer_is_valid(const struct codec_ack_packet *packet);
static bool codec_data_packet_header_is_valid(const uint8_t *buffer, size_t length);

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
/* ACK-FR-003/005, ACK-SR-001/002/003: exact wire length and opcode. */
static bool codec_ack_buffer_is_valid(const struct codec_ack_packet *packet) {
    // Validate pointers and exact wire length before reading (ACK-SR-001/002/003).
    return packet != NULL &&
           codec_buffer_is_valid(packet->buffer, packet->length, TFTP_OPCODE_ACK,
                                 TFTP_ACK_PACKET_LENGTH, TFTP_ACK_PACKET_LENGTH);
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
    // DATA-FR-002/003/004/009: four-byte header and at most 512 payload bytes.
    return codec_buffer_is_valid(buffer, length, TFTP_OPCODE_DATA,
                                 TFTP_DATA_PACKET_HEADER_LENGTH,
                                 TFTP_DATA_PACKET_HEADER_LENGTH + TFTP_DATA_PAYLOAD_MAX_LENGTH);
}

#pragma endregion "DATA Decoder Implementation"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma region "ERROR Decoder Implementation"

/* RFC 1350 section 5, Figure 5-4. Require exactly one terminated string.
 * Validate bounds before examining fields; no writes occur on rejection.
 */
static bool codec_error_buffer_is_valid(const uint8_t *buffer, size_t length) {
    // ERROR-FR-001/002/004/005/007/009: header, then one possibly empty
    // terminated message consuming the remaining buffer, with no size cap.
    return codec_buffer_is_valid(buffer, length, TFTP_OPCODE_ERROR, 5, SIZE_MAX) &&
           codec_string_end(buffer, length, 4, true) != NULL;
}

/**
 * @brief Decodes a TFTP ERROR packet.
 *
 * @details
 * Wire layout follows RFC 1350, Section 5, Figure 5-4.
 * Requirement IDs below are local project identifiers.
 *
 * Wire-format requirements:
 * - ERROR-FR-001: Packet must be at least 5 bytes, including the terminator.
 * - ERROR-FR-002: Opcode must equal 5.
 * - ERROR-FR-003: Decode the 16-bit error code in network byte order.
 * - ERROR-FR-004: Message begins at byte offset 4 and is NUL-terminated.
 *
 * Local API and validation requirements:
 * - ERROR-FR-005: Require the first message NUL to be the final packet byte.
 * - ERROR-FR-006: Expose message length excluding the terminator (length - 5).
 * - ERROR-FR-007: Accept empty messages and all 16-bit codes, including unknowns.
 * - ERROR-FR-008: Preserve message bytes without netascii validation/conversion.
 * - ERROR-FR-009: Do not apply the DATA payload's 512-byte limit to ERROR.
 * - ERROR-SR-001: Reject NULL input buffer.
 * - ERROR-SR-002: Reject NULL output pointer.
 * - ERROR-SR-003: Never read beyond the supplied buffer; bound the NUL search.
 * - ERROR-SR-004: Do not modify any output fields on failure.
 * - ERROR-SR-005: Do not modify input bytes.
 * - ERROR-SR-006: Borrow the message within the input buffer without allocation.
 *
 * @param packet Buffer, length, and decoded error code/message fields.
 * @return true on success, false on failure.
 */
bool decode_error(struct codec_error_packet *packet) {
    // Reject NULL output and finish validation before writing any fields
    // (ERROR-SR-002/004).
    if (packet == NULL || !codec_error_buffer_is_valid(packet->buffer, packet->length)) {
        return false;
    }
    // Decode network byte order without restricting the 16-bit code range
    // (ERROR-FR-003/007).
    packet->error_code = get_error_code(packet->buffer);
    // Borrow the original message at offset 4 without copying or modifying it
    // (ERROR-FR-004/008, ERROR-SR-005/006).
    packet->error_message = (const char *)(packet->buffer + 4);
    // Exclude the four-byte header and final NUL (ERROR-FR-006).
    packet->error_message_length = packet->length - 5;
    return true;
}

bool populate_error_packet(struct codec_error_packet *packet,
                           const uint8_t *buffer, size_t length) {
    // Reuse wire validation and reject NULL output before initialization;
    // rejection leaves every destination field unchanged (ERROR-SR-002/004).
    if (packet == NULL || !codec_error_buffer_is_valid(buffer, length)) {
        return false;
    }
    // Initialize borrowed storage without modifying input (ERROR-SR-005/006).
    // Decoded fields remain zero/NULL until decode_error succeeds.
    const struct codec_error_packet candidate = {buffer, length, 0, NULL, 0};
    *packet = candidate;
    return true;
}

#pragma endregion "ERROR Decoder Implementation"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma region "Shared Request Decoder Helpers"

/* Validate before writing so malformed requests never partially update the
 * caller's packet. Bound both terminator searches to the supplied buffer.
 */
static bool codec_decode_request(struct codec_request_block *packet,
                                 uint16_t opcode) {
    if (packet == NULL ||
        !codec_buffer_is_valid(packet->buffer, packet->length, opcode, 6, SIZE_MAX)) {
        return false;
    }

    const uint8_t *filename = packet->buffer + 2;
    const uint8_t *filename_end = codec_string_end(packet->buffer, packet->length, 2, false);
    if (filename_end == NULL || filename_end == filename) {
        return false;
    }

    const uint8_t *mode = filename_end + 1;
    const uint8_t *mode_end = codec_string_end(packet->buffer, packet->length,
                                              (size_t)(mode - packet->buffer), true);
    if (mode_end == NULL || mode_end == mode) {
        return false;
    }

    packet->opcode = opcode;
    packet->filename = filename;
    packet->filename_length = (size_t)(filename_end - filename);
    packet->mode = mode;
    packet->mode_length = (size_t)(mode_end - mode);
    return true;
}

static bool codec_populate_request(struct codec_request_block *packet,
                                   const uint8_t *buffer, size_t length,
                                   uint16_t opcode) {
    struct codec_request_block candidate = {buffer, length, 0, NULL, 0, NULL, 0};
    if (packet == NULL || !codec_decode_request(&candidate, opcode)) {
        return false;
    }
    const struct codec_request_block initial = {buffer, length, 0, NULL, 0, NULL, 0};
    *packet = initial;
    return true;
}

#pragma endregion "Shared Request Decoder Helpers"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma region "RRQ Decoder Implementation"

bool decode_rrq(struct codec_request_block *packet) {
    return codec_decode_request(packet, TFTP_OPCODE_RRQ);
}

bool populate_rrq_packet(struct codec_request_block *packet,
                         const uint8_t *buffer, size_t length) {
    return codec_populate_request(packet, buffer, length, TFTP_OPCODE_RRQ);
}

#pragma endregion "RRQ Decoder Implementation"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma region "WRQ Decoder Implementation"

bool decode_wrq(struct codec_request_block *packet) {
    return codec_decode_request(packet, TFTP_OPCODE_WRQ);
}

bool populate_wrq_packet(struct codec_request_block *packet,
                         const uint8_t *buffer, size_t length) {
    return codec_populate_request(packet, buffer, length, TFTP_OPCODE_WRQ);
}

#pragma endregion "WRQ Decoder Implementation"
