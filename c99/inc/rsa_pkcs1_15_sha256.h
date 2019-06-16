#ifndef __RSA_PKCS1_15_SHA256_H__
#define __RSA_PKCS1_15_SHA256_H__

#define SHA256_ONLY_LE
#include "sha256.h"
#define DIGEST_LEN 32

#include "min-bn-math.h"
//#include "min-bn-io.h"
#define N_BYTE_LENGTH (BN_SAFE_WIDTH/8)

//return 0 if sig match
static int rsa_verify_pkcs1_15_sha256(
    const void*const message,
    uint16_t size,
    const BN_WORD*const sig,
    BN_WORD e[BN_WORDS],
    BN_WORD n[BN_WORDS]
){
    BN_WORD x[BN_WORDS] = {0};
    BN_WORD y[BN_WORDS];

    sha256_sum_little(message,size,x);
    const uint8_t pad[] = {
        0x20,0x04,0x00,0x05,
        0x01,0x02,0x04,0x03,
        0x65,0x01,0x48,0x86,
        0x60,0x09,0x06,0x0d,
        0x30,0x31,0x30,0x00,
    };
    uint8_t*x8=(uint8_t*)x;
    memcpy(x8+DIGEST_LEN,pad,sizeof(pad));
    const unsigned int ffpadoffset = DIGEST_LEN+sizeof(pad);
    memset(x8+ffpadoffset,0xFF,N_BYTE_LENGTH-2-ffpadoffset);
    x8[N_BYTE_LENGTH-2]=0x01;
    x8[N_BYTE_LENGTH-1]=0x00;
    //printf("\nsizeof(BN_WORD)=%lu\n\n",sizeof(BN_WORD));
    //bn_dumphex("e=",e);
    //bn_dumphex("n=",n);
    //bn_dumphex("sig=",sig);
    bn_modexp(y, sig,e,n);
    int status = memcmp(y,x,sizeof(y));
    //bn_dumphex("y=",y);
    //if(status){
    //    printf("sig mismatch:\n");
    //    println_bytes("computed digest:  ",x,sizeof(x));
    //    println_bytes("decrypted digest: ",y,sizeof(y));
    //}else{
    //    printf("sig match\n");
    //}
    return status;
}

#endif
