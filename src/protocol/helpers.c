#include "utils/codec_helpers.h"
#include <string.h>

#pragma region "Wire Buffer Validation"

bool codec_buffer_is_valid(const uint8_t *buffer, size_t length,
                           uint16_t opcode, size_t minimum, size_t maximum) {
    return length >= minimum && length <= maximum &&
           has_opcode(buffer, length, opcode);
}

#pragma endregion "Wire Buffer Validation"

#pragma region "Bounded String Parsing"

const uint8_t *codec_string_end(const uint8_t *buffer, size_t length,
                                size_t offset, bool require_final) {
    if (buffer == NULL || offset >= length) {
        return NULL;
    }
    const uint8_t *end = memchr(buffer + offset, 0, length - offset);
    if (end == NULL || (require_final && end != buffer + length - 1)) {
        return NULL;
    }
    return end;
}

#pragma endregion "Bounded String Parsing"
