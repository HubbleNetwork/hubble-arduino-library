/*
 * The PSA crypto implementation (in crypto/psa.c) #includes <psa/crypto.h> which is
 * not included by PSACrypto.h. psa.c also needs to include PSACrypto.h for Arduino,
 * so create this shim to include the Arduino version.
 */
#include <PSACrypto.h>
