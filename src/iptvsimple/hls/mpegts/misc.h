#ifndef __HLS_DownLoad__misc__
#define __HLS_DownLoad__misc__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct write_ctx {
    size_t (*write) ( const uint8_t *buf, size_t len, void *opaque);
    void *opaque;
} write_ctx_t;

#ifdef __cplusplus
}
#endif

#endif /* defined(__HLS_DownLoad__misc__) */
