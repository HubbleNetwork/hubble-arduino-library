/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino implementation of the Hubble SDK platform port.
 */

/* Include config file first. Ensures files can access the set configs. */
#include "hubble_config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>

#include <hubble/port/sys.h>

extern "C" {

uint64_t hubble_uptime_get(void)
{
    return (uint64_t)millis();
}

int hubble_log(enum hubble_log_level level, const char *format, ...)
{
	static const char *const level_tag[HUBBLE_LOG_COUNT] = {
		"<dbg>", "<inf>", "<wrn>", "<err>"};
	char buf[128];
	va_list args;

	/* Unsigned compare catches an out-of-range level in either direction. */
	if ((unsigned int)level >= (unsigned int)HUBBLE_LOG_COUNT) {
		return -1;
	}

#ifdef CONFIG_HUBBLE_ARDUINO_LOG_LEVEL
	if (((unsigned int)level + CONFIG_HUBBLE_ARDUINO_LOG_LEVEL) <
	    (unsigned int)HUBBLE_LOG_COUNT) {
		return 0;
	}
#endif

	va_start(args, format);
	(void)vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	Serial.print("hubble ");
	Serial.print(level_tag[level]);
	Serial.print(' ');
	Serial.print(buf);

	/* Only append newline if it is missing. */
	size_t len = strlen(buf);
	if ((len == 0) || (buf[len - 1] != '\n')) {
		Serial.println();
	}

	return 0;
}

/* Hubble locks are stubbed for now. */
int hubble_lock_init(void)
{
	return 0;
}

void hubble_lock(void)
{
}

void hubble_unlock(void)
{
}

} /* extern "C" */
