#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "aes.h"
#include "msg.h"

void* AES128_CBC_CTX_new(void)
{
  return EVP_CIPHER_CTX_new();
}

int AES128_CBC_DecryptInit(void *ctx, uint8_t *key, uint8_t *iv, bool with_padding)
{
  int ret = EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_set_padding(ctx, with_padding ? 1 : 0); // https://github.com/openssl/openssl/issues/3516,
  return ret;
}

int AES128_CBC_DecryptUpdate(void *ctx, uint8_t *plaintext, uint8_t *ciphertext, int in_size)
{
  int out_size = 0;
  int ret = 1;

  if (in_size > 0)
  {
    ret = EVP_DecryptUpdate(ctx, plaintext, &out_size, ciphertext, in_size);
    if (1 != ret || in_size != out_size)
    {
      MSG_ERROR("AES128_CBC_DecryptUpdate failed: %d, in_size: %d, out_size: %d\n", ret, in_size, out_size);
      exit(1);
    }
  }
  return ret;
}

int AES128_CBC_DecryptPadded(void *ctx, uint8_t *plaintext, uint8_t *ciphertext, int in_size, int *out_size)
{
  int ret = 1;

  if (in_size > 0)
  {
    ret = EVP_DecryptUpdate(ctx, plaintext, (int *)out_size, ciphertext, in_size);
    if (1 == ret) {
        int out_size2 = 0;
        ret = EVP_DecryptFinal_ex(ctx, plaintext + *out_size, &out_size2);
        *out_size += 1 == ret ? out_size2 : 0;
    }
    if (1 != ret) { // out size could be smaller then in because of padding
        // this could fail because of temporary server problem, do not be so brutal to exit in such case
        MSG_ERROR("AES128_CBC_DecryptUpdate failed: %d, in_size: %d, out_size: %d\n", ret, in_size, *out_size);
        exit(1);
    }
  }

  return ret;
}

void AES128_CBC_free(void *ctx)
{
  EVP_CIPHER_CTX_free(ctx);
}