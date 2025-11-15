#include <Arduino.h>
#include <hubble/port/sys.h>

uint64_t hubble_uptime_get(void)
{
    return (uint64_t)millis();
}

int hubble_log(enum hubble_log_level level, const char *format, ...)
{
    return 0;
}