/*
 * Conditionally include the correct crypto backend based on architecture.
 */
#define CONFIG_HUBBLE_KEY_SIZE 32
#define CONFIG_HUBBLE_NETWORK_KEY_256
#define CONFIG_HUBBLE_BLE_NETWORK_TIMER_COUNTER_DAILY

#if defined(ARDUINO_ARCH_MBED)
#include "../hubble_sdk/src/crypto/mbedtls.c"
#else // defined(ARDUINO_ARCH_MBED)
#include "../hubble_sdk/src/crypto/psa.c"
#endif

#include "../hubble_sdk/src/hubble_ble.c"
