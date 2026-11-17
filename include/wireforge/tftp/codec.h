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

/**
 * Decode a four-byte ACK packet with opcode 4 into a host-order block number.
 * Returns true after writing ack_block->block_number; returns false for NULL
 * pointers, invalid length, or invalid opcode, leaving supplied output unchanged.
 * Non-NULL buffer must refer to at least length readable bytes; non-NULL
 * ack_block must point to a writable structure. No transfer-state checks occur.
 */
bool decode_ack(
    const uint8_t *buffer,
    size_t length,
    struct codec_ack_block *ack_block
);

#endif /* WIREFORGE_TFTP_CODEC_H */
