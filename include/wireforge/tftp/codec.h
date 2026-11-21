#ifndef WIREFORGE_TFTP_CODEC_H
#define WIREFORGE_TFTP_CODEC_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct codec_packet {
    const uint8_t *buffer;
    size_t length;
    uint16_t block_number;
};

/*
 * Decode a host-order block number after validating length and opcode.
 * Reject NULL packet/buffer. Failure leaves all fields unchanged.
 * ACK requires 4 bytes; standard DATA requires 4 through 516 bytes.
 * Caller supplies writable packet storage and length readable buffer bytes.
 * Buffers are borrowed and never modified. DATA payload fields are deferred.
 */
bool decode_ack(struct codec_packet *packet);
bool decode_data(struct codec_packet *packet);

/*
 * Validate and initialize buffer, length, and block_number=0 without decoding.
 * Failure leaves packet unchanged. Buffer must remain readable during decoding.
 */
bool populate_ack_packet(struct codec_packet *packet,
                         const uint8_t *buffer, size_t length);
bool populate_data_packet(struct codec_packet *packet,
                          const uint8_t *buffer, size_t length);
#endif /* WIREFORGE_TFTP_CODEC_H */
