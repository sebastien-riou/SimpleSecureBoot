#ifndef __SSB_H__
#define __SSB_H__

typedef uint32_t BN_WORD;
#include "ssb_pub_key.h"
#include "rsa_pkcs1_15_sha256.h"

//return 0 if sig match
static int ssb_check_sig(const void*const buf, uint16_t size, const BN_WORD*const sig){
    BN_WORD e[BN_WORDS] = {0};
	BN_WORD n[BN_WORDS] = {0};
    memcpy(e,&ssb_e,sizeof(ssb_e));
    memcpy(n,ssb_n,sizeof(ssb_n));
    return rsa_verify_pkcs1_15_sha256(buf,size,sig,e,n);
}

#endif
