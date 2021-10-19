//Simple secure bootloader
#pragma once

/*
Image structure:
- Total size: 8 bytes
- Magic word/version: 8 bytes
- Load address: 8 bytes
- Start address: 8 bytes
- Data
- Signature of all the above
*/

#include "ssb.h"
#include <setjmp.h>

//shall be declared by higher level file
//static jmp_buf ssbl_exception_ctx;

#ifndef SSBL_CUSTOM_IO
static uint64_t ssbl_read64(){
    uint64_t out=0;
    for(unsigned int i=0;i<8;i++){
        uint64_t b = getchar();
        out |= b<<(i*8);
    }
    return out;
}
static void ssbl_mem_write64(uint64_t*addr, uint64_t dat){
    *addr = dat;
}
#endif

#define WORDS_PER_U64 (sizeof(uint64_t) / sizeof(BN_WORD))
#define DIV_CEIL(a,b) (((a) + ((b)-1)) / (b))
#if 0 //( sizeof(uint64_t) == sizeof(BN_WORD) )
    #define BN_WORDS64 BN_WORDS
#else
    #define BN_WORDS64 DIV_CEIL(BN_WORDS, WORDS_PER_U64)
#endif

#define SSBL_MAGIC "ssbl001"

typedef struct ssbl_dat_reader_ctx_struct {
    uint64_t package_size;
    uint64_t magic;
    uint64_t load_address;
    uint64_t start_address;
    uint64_t data_size;
    uint64_t state;
    const void*dat;
} ssbl_dat_reader_ctx_t;

static size_t ssbl_dat_reader(void*ctx, const void**dat){
    ssbl_dat_reader_ctx_t*c = (ssbl_dat_reader_ctx_t*)ctx;
    size_t out;
    switch(c->state){
        case 0:
            *dat = ctx;
            out = 32;
            c->state=1;
            break;
        case 1:
            *dat = c->dat;
            out = c->data_size;
            c->state=2;
            break;
        default:
            out=0;
            break;
    }
    //printf("dat_reader dat=%p, out=%lx\n",dat,out);
    //fflush(stdout);
    return out;
}

int ssbl_main(uint64_t base, uint64_t size){
    uint64_t sig[BN_WORDS64]={0};
    const uint64_t package_size = ssbl_read64();
    const uint64_t magic = ssbl_read64();
    const uint64_t load_address = ssbl_read64();
    const uint64_t start_address = ssbl_read64();
    const uint64_t data_size = package_size - 32 - sizeof(sig);

    debug_println64x("INFO: package_size  0x",package_size);
    debug_println64x("INFO: magic         0x",magic);
    debug_println64x("INFO: load_address  0x",load_address);
    debug_println64x("INFO: start_address 0x",start_address);
    debug_println64x("INFO: data_size     0x",data_size);
            
    //sanity checks
    uint64_t expected_magic;
    memcpy(&expected_magic,SSBL_MAGIC,sizeof(expected_magic));
    ASSERT_EQ_VAL(expected_magic,magic);
    ASSERT_EQ_VAL(0,package_size % sizeof(uint64_t));
    ASSERT_EQ_VAL(0,load_address % sizeof(uint64_t));
    ASSERT_EQ_VAL(0,data_size % sizeof(uint64_t));

    assert(size >= data_size);
    assert(base <= load_address);
    assert(base+size >= load_address+data_size);

    uint64_t*load_address_ptr = (uint64_t*)(uintptr_t)load_address;

    //store data in internal memory
    uint64_t*dst = load_address_ptr;
    for(uint64_t i=0;i<data_size/sizeof(uint64_t);i++){
        ssbl_mem_write64(dst+i,ssbl_read64());
    }

    //store signature
    dst = sig;
    for(uint64_t i=0;i<sizeof(sig);i+=sizeof(uint64_t)){
        *dst++ = ssbl_read64();
    }
    debug_println_bytes("INFO: signature\n",sig,sizeof(sig));
    
    ssbl_dat_reader_ctx_t c;
    c.package_size = package_size;
    c.magic = magic;
    c.load_address = load_address;
    c.start_address = start_address;
    c.data_size = data_size;
    c.dat = load_address_ptr;
    c.state=0;
    
    int status = ssb_check_sig(ssbl_dat_reader,&c,(BN_WORD*)sig);
    if(status){
        memset(load_address_ptr,0,data_size);
        debug_println("ERROR: sig mismatch, data erased");
        return -1;
    }else{
        if(start_address+1){
            debug_println64x("INFO: sig match, calling 0x",start_address);
            void (*target)(void) = (void (*)(void))(uintptr_t)start_address;
            target();
        } else {
            debug_println("INFO: sig match, data loaded");
        }
        return 0;
    }
}

static int ssbl_main_loop(uint64_t base, uint64_t size){
  int status = 0;
  while(0<=status){//negative is fatal error
    uint32_t errcode;
    if (0 == (errcode = setjmp(ssbl_exception_ctx))){
      status = ssbl_main(base, size);
    }else{
      status = errcode;
    }
  }
  return status;
}