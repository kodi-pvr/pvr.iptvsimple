#ifndef _HLSDL_AES_CRYPTO_H_
#define _HLSDL_AES_CRYPTO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void * AES128_CBC_CTX_new(void);

int AES128_CBC_DecryptInit(void *ctx, uint8_t *key, uint8_t *iv, bool with_padding);

int AES128_CBC_DecryptUpdate(void *ctx, uint8_t *plaintext, uint8_t *ciphertext, int in_size);

int AES128_CBC_DecryptPadded(void *ctx, uint8_t *plaintext, uint8_t *ciphertext, int in_size, int *out_size);

void AES128_CBC_free(void *ctx);

#ifdef __cplusplus
}
#endif

#endif //_HLSDL_AES_CRYPTO_H_