#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t b64_decoded_size(const char *in);
int b64_decode(const char *in, uint8_t *out, size_t outlen);

#ifdef __cplusplus
}
#endif
