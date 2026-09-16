/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Base64 decoding for master keys.
 *
 */

#ifndef HUBBLE_B64_H
#define HUBBLE_B64_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode a base64 string into exactly @p outlen bytes.
 *
 * Whitespace is skipped and trailing '=' padding is optional, so a key
 * pasted out of a dashboard or a terminal decodes whether or not it picked
 * up a newline or lost its padding on the way.
 *
 * The decoded length must match @p outlen exactly, so a 128-bit key pasted
 * into a 256-bit build is rejected rather than producing a device that
 * transmits but never decodes.
 *
 * @param in     NUL-terminated base64 string.
 * @param out    Destination buffer.
 * @param outlen Exact byte count expected, normally CONFIG_HUBBLE_KEY_SIZE.
 *
 * @retval 0       On success.
 * @retval -EINVAL On a NULL argument, an invalid character, a truncated
 *                 group, or a decoded length other than @p outlen. The
 *                 buffer may have been partially written.
 */
int b64_decode(const char *in, uint8_t *out, size_t outlen);

#ifdef __cplusplus
}
#endif

#endif /* HUBBLE_B64_H */
