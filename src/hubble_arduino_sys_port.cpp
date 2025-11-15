#include <Arduino.h>
#include <hubble/port/sys.h>
#include <stdarg.h>

uint64_t hubble_uptime_get(void)
{
    return (uint64_t)millis();
}

static const char *level_to_str(enum hubble_log_level level) {
    switch (level) {
        case HUBBLE_LOG_DEBUG:   return "DEBUG";
        case HUBBLE_LOG_INFO:    return "INFO";
        case HUBBLE_LOG_WARNING: return "WARN";
        case HUBBLE_LOG_ERROR:   return "ERROR";
        default:                 return "?";
    }
}

int hubble_log(enum hubble_log_level level, const char *format, ...)
{
    char buf[128];  // adjust size as needed
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    // vsnprintf returns number of chars that *would* have been written
    if (n < 0) {
        return n;
    }

    // Optional: timestamp from millis()
    unsigned long ms = millis();

    Serial.print('[');
    Serial.print(ms);
    Serial.print(" ms]");
    Serial.print(" [");
    Serial.print(level_to_str(level));
    Serial.print("] ");

    Serial.println(buf);

    return 0;
}