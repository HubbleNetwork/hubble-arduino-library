/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "b64.h"

#include <errno.h>
#include <stdbool.h>

static const int8_t _b64_invs[] = {
	62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1,
	-1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
	-1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
	36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

static int _b64_value(char c)
{
	unsigned char uc = (unsigned char)c;

	if ((uc < (unsigned char)'+') || (uc > (unsigned char)'z')) {
		return -1;
	}

	return _b64_invs[uc - '+'];
}

static bool _b64_is_space(char c)
{
	return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') ||
	       (c == '\v') || (c == '\f');
}

int b64_decode(const char *in, uint8_t *out, size_t outlen)
{
	uint32_t acc = 0U;
	unsigned int bits = 0U;
	unsigned int group = 0U;
	bool padded = false;
	size_t n = 0U;

	if ((in == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	for (const char *p = in; *p != '\0'; p++) {
		int v;

		if (_b64_is_space(*p)) {
			continue;
		}

		if (*p == '=') {
			/* Padding only valid in last two positions of a four-digit group. */
			if (group < 2U) {
				return -EINVAL;
			}

			padded = true;
			group = (group + 1U) & 3U;
			continue;
		}

		/* Padding ends the string; nothing may follow it. */
		if (padded) {
			return -EINVAL;
		}

		v = _b64_value(*p);
		if (v < 0) {
			return -EINVAL;
		}

		acc = (acc << 6) | (uint32_t)v;
		bits += 6U;
		group = (group + 1U) & 3U;

		if (bits >= 8U) {
			bits -= 8U;

			if (n >= outlen) {
				return -EINVAL;
			}

			out[n++] = (uint8_t)((acc >> bits) & 0xFFU);
		}
	}

	/* One digit carries 6 bits: too few for a byte, so it was truncated. */
	if (group == 1U) {
		return -EINVAL;
	}

	/* An exact fit is required. */
	if (n != outlen) {
		return -EINVAL;
	}

	return 0;
}
