#ifndef __RSA_PKCS1_15_SHA256_H__
#define __RSA_PKCS1_15_SHA256_H__

#define SHA256_CALLBACK
#include "sha256.h"
#define DIGEST_LEN 32


//#define BN_MATH_DEBUG
#include "min-bn-math.h"
#define N_BYTE_LENGTH (BN_SAFE_WIDTH/8)

//return 0 if sig match
static int rsa_verify_pkcs1_15_sha256(
    dat_reader_t dat_reader, 
    void*dat_reader_ctx,
    const BN_WORD*const sig,
    const BN_WORD e[BN_WORDS],
    const BN_WORD n[BN_WORDS],
    const BN_WORD R[BN_WORDS],
    const BN_WORD R2[BN_WORDS],
    unsigned int mp
){
    (void)bn_remove_unused_warnings;
    #ifndef RSA_PKCS1_15_EXTXY
    BN_WORD rsa_x[BN_WORDS] = {0};
    BN_WORD rsa_y[BN_WORDS] = {0};
    #endif
    BN_WORD safe_sig[BN_WORDS] = {0};
    const unsigned int SIZEOFX = sizeof(safe_sig);
    const unsigned int SIZEOFY = sizeof(safe_sig);
    (void)SIZEOFX;
    memcpy(safe_sig,sig,N_BYTE_LENGTH);
    
    sha256_sum_callback(dat_reader,dat_reader_ctx,rsa_x);

    const uint8_t pad[] = {
        0x20,0x04,0x00,0x05,
        0x01,0x02,0x04,0x03,
        0x65,0x01,0x48,0x86,
        0x60,0x09,0x06,0x0d,
        0x30,0x31,0x30,0x00,
    };
    uint8_t*x8=(uint8_t*)rsa_x;
    memcpy(x8+DIGEST_LEN,pad,sizeof(pad));
    const unsigned int ffpadoffset = DIGEST_LEN+sizeof(pad);
    memset(x8+ffpadoffset,0xFF,N_BYTE_LENGTH-2-ffpadoffset);
    x8[N_BYTE_LENGTH-2]=0x01;
    x8[N_BYTE_LENGTH-1]=0x00;
    #ifdef BN_MATH_DEBUG
        printf("\nsizeof(BN_WORD)=%lu\n\n",sizeof(BN_WORD));
        bn_dumphex("e=",e);
        bn_dumphex("n=",n);
        bn_dumphex("sig=",safe_sig);
    #endif
    bn_modexp_montgommery(rsa_y, safe_sig,e,n,R,R2,mp);
    int status = memcmp(rsa_y,rsa_x,SIZEOFY);
    #ifdef BN_MATH_DEBUG
        bn_dumphex("y=",rsa_y);
        if(status){
            printf("sig mismatch:\n");
            println_bytes("computed digest:  ",rsa_x,SIZEOFX);
            println_bytes("decrypted digest: ",rsa_y,SIZEOFY);
        }else{
            printf("sig match\n");
        }
    #endif
    return status;
}

#endif
