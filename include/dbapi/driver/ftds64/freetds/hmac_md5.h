#ifndef HMAC_MD5_H
#define HMAC_MD5_H

/* $Id: hmac_md5.h 103491 2007-05-04 17:18:18Z kazimird $ */

#include "md5.h"

typedef struct
{
    MD5_CTX         ctx;
    unsigned char   k_ipad[65];
    unsigned char   k_opad[65];
} HMACMD5Context;

void hmac_md5_init_limK_to_64(const unsigned char* key,
                              int key_len,
                              HMACMD5Context *ctx);
void hmac_md5_update(const unsigned char* text,
                     int text_len,
                     HMACMD5Context *ctx);
void hmac_md5_final(unsigned char* digest,
                    HMACMD5Context *ctx);
void hmac_md5(const unsigned char key[16],
              const unsigned char* data,
              int data_len,
              unsigned char* digest);
void hmac_md5_init_rfc2104(const unsigned char* key,
                           int key_len,
                           HMACMD5Context *ctx);

#endif /* !HMAC_MD5_H */
