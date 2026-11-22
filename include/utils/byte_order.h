#ifndef WIREFORGE_UTILS_BYTE_ORDER_H
#define WIREFORGE_UTILS_BYTE_ORDER_H

#include <stdint.h>

/// Read a 16-bit unsigned integer from a buffer in big-endian order.
/// Caller must provide at least two readable bytes.
static inline uint16_t read_u16_be(const uint8_t *buffer) {
    return (uint16_t)(((uint16_t)buffer[0] << 8) | buffer[1]);
}

#endif /* WIREFORGE_UTILS_BYTE_ORDER_H */
