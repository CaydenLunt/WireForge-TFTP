#ifndef WIREFORGE_TFTP_CODEC_H
#define WIREFORGE_TFTP_CODEC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct codec_ack_block {
    // ACK-FR-007 (representation): uint16_t provides exactly the range
    // 0 through 65535, including zero; no runtime range check is needed.
    // Network-byte-order decoding (ACK-FR-006) is still the decoder's job.
    uint16_t block_number;
};

bool decode_ack(
    const uint8_t *buffer,
    size_t length,
    struct codec_ack_block *ack_block
);

#endif /* WIREFORGE_TFTP_CODEC_H */
