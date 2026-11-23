#ifndef WIREFORGE_TFTP_CODEC_H
#define WIREFORGE_TFTP_CODEC_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct codec_ack_packet {
    const uint8_t *buffer;
    size_t length;
    uint16_t block_number;
};

struct codec_data_packet {
    const uint8_t *buffer;
    size_t length;
    uint16_t block_number;
    const uint8_t *payload;
    size_t payload_length;
};

struct codec_error_packet {
    const uint8_t *buffer;
    size_t length;
    uint16_t error_code;
    const char *error_message;
    size_t error_message_length;
};

struct codec_request_block {
    const uint8_t *buffer;
    size_t length;
    uint16_t opcode;
    const uint8_t *filename;
    size_t filename_length;
    const uint8_t *mode;
    size_t mode_length;
};

/*
 * Decode a host-order block number after validating length and opcode.
 * Reject NULL packet/buffer. Failure leaves all fields unchanged.
 * ACK requires 4 bytes; standard DATA requires 4 through 516 bytes.
 * Caller supplies writable packet storage and length readable buffer bytes.
 * Buffers are borrowed and never modified. DATA exposes payload = buffer + 4
 * and payload_length = length - 4. For empty DATA, payload is one-past-end
 * and must not be dereferenced. ACK has no payload fields.
 */
bool decode_ack(struct codec_ack_packet *packet);
bool decode_data(struct codec_data_packet *packet);
/* Requests contain a nonempty filename and mode, each NUL-terminated.
 * The mode terminator must end the packet (option extensions are unsupported).
 * Mode bytes are preserved; supported transfer modes are a caller policy.
 * Decoding borrows both strings and reports lengths excluding terminators.
 * Populate validates and initializes decoded fields to zero/NULL.
 * Both operations reject NULL pointers and preserve all fields on failure.
 * Caller keeps length readable buffer bytes alive while using these views.
 */
bool decode_rrq(struct codec_request_block *packet);
bool decode_wrq(struct codec_request_block *packet);

/*
 * Validate and initialize buffer, length, and block_number=0 without decoding.
 * DATA payload fields are initialized to NULL/zero until decoding.
 * Failure leaves packet unchanged. Buffer must remain readable while decoding
 * and while accessing the borrowed payload.
 */
bool populate_ack_packet(struct codec_ack_packet *packet,
                         const uint8_t *buffer, size_t length);
bool populate_data_packet(struct codec_data_packet *packet,
                          const uint8_t *buffer, size_t length);
bool populate_rrq_packet(struct codec_request_block *packet,
                         const uint8_t *buffer, size_t length);
bool populate_wrq_packet(struct codec_request_block *packet,
                         const uint8_t *buffer, size_t length);
/*
 * ERROR requires opcode 5, at least five bytes, and a message whose first NUL
 * is the final packet byte. Empty messages and all 16-bit error codes are
 * accepted. Message bytes are preserved without netascii validation/conversion;
 * no DATA-style 512-byte limit is imposed.
 * decode_error exposes a borrowed, NUL-terminated message at buffer + 4 and
 * its length excluding the terminator, plus the host-order error code.
 * populate_error_packet validates then initializes code=0, message=NULL,
 * message_length=0. Both reject NULL packet/buffer and preserve all fields on
 * failure. Caller supplies length readable bytes and keeps the buffer alive
 * while decoding or accessing the message. Neither function modifies input.
 */
bool decode_error(struct codec_error_packet *packet);
bool populate_error_packet(struct codec_error_packet *packet,
                           const uint8_t *buffer, size_t length);
#endif /* WIREFORGE_TFTP_CODEC_H */

