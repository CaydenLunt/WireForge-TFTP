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
#endif /* WIREFORGE_TFTP_CODEC_H */
