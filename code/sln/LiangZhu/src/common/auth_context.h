#ifndef __HILUO_AUTH_CONTEXT_INCLUDE_H__
#define __HILUO_AUTH_CONTEXT_INCLUDE_H__

#include "common.h"


/* -- START: Defines for OpenSSL -- */
typedef SHA_CTX               SHAHashStateType;
#define SHA1_INIT(s)          SHA1_Init(s)
#define SHA1_PROCESS(s, p, l) SHA1_Update(s, p, l)
#define SHA1_DONE(s, k)       SHA1_Final(k, s)

typedef MD5_CTX               MD5HashStateType;
#define MD5_INIT(s)           MD5_Init(s)
#define MD5_PROCESS(s, p, l)  MD5_Update(s, p, l)
#define MD5_DONE(s, k)        MD5_Final(k, s)

typedef des_key_schedule      DESCBCType;
#define DES_CBC_START_ENCRYPT(c, iv, k, kl, r, s) \
if (des_key_sched((C_Block*)(k), s) < 0) \
{ \
    return SNMPv3_USM_ERROR; \
}
#define DES_CBC_START_DECRYPT(c, iv, k, kl, r, s) \
if (des_key_sched((C_Block*)(k), s) < 0) \
{ \
    return SNMPv3_USM_ERROR; \
}

#define DES_CBC_ENCRYPT(pt, ct, s, iv, l) \
    des_ncbc_encrypt(pt, ct, l, \
    s, (C_Block*)(iv), DES_ENCRYPT)
#define DES_CBC_DECRYPT(ct, pt, s, iv, l) \
    des_ncbc_encrypt(ct, pt, l, \
    s, (C_Block*)(iv), DES_DECRYPT)

#define DES_EDE3_CBC_ENCRYPT(pt, ct, l, k1, k2, k3, iv) \
    des_ede3_cbc_encrypt(pt, ct, l, \
    k1, k2, k3, (C_Block*)(iv), DES_ENCRYPT)

#define DES_EDE3_CBC_DECRYPT(ct, pt, l, k1, k2, k3, iv) \
    des_ede3_cbc_encrypt(ct, pt, l, \
    k1, k2, k3, (C_Block*)(iv), DES_DECRYPT)

#define DES_MEMSET(s, c, l)   memset(&(s), c, l)


#endif
