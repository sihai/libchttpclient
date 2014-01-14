
#ifndef MD5_H_
#define MD5_H_

#include "type.h"

typedef struct {
    uint64_t  bytes;
    uint32_t  a, b, c, d;
    u_char    buffer[64];
} md5_context;


void md5_init(md5_context *ctx);
void md5_update(md5_context *ctx, const void *data, size_t size);
void md5_final(u_char result[16], md5_context *ctx);

void md5(const void *data, size_t size, char* output);

#endif
