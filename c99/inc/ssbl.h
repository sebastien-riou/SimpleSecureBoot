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

static uint64_t ssbl_mask;
#define RSA_PKCS1_15_EXTXY
typedef uint32_t BN_WORD;
static BN_WORD *rsa_x;
static BN_WORD *rsa_y;
#include "ssb.h"
#include <setjmp.h>

//shall be declared by higher level file
//static jmp_buf ssbl_exception_ctx;

#ifndef SSBL_PREPARE_LOAD
#define SSBL_PREPARE_LOAD()
#endif
#ifndef SSBL_PREPARE_CALL
#define SSBL_PREPARE_CALL()
#endif

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
    *addr = dat;//opportunity to write in another way
}
static const uint64_t*const ssbl_mem_readbuf(const uint64_t* const addr){
    return addr;//opportunity to do read address translation
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

static uint64_t ssbl_dat_reader_buf[8];
static size_t ssbl_dat_reader(void*ctx, const void**dat){
    ssbl_dat_reader_ctx_t*c = (ssbl_dat_reader_ctx_t*)ctx;
    size_t out;
    static uint32_t remaining;
    static const uint64_t* src;
    switch(c->state){
        case 0:
            *dat = ctx;
            out = 32;
            c->state=1;
            remaining=c->data_size;
            src = ssbl_mem_readbuf(c->dat);
            break;
        case 1:
            *dat = ssbl_dat_reader_buf;
            if(remaining<=sizeof(ssbl_dat_reader_buf)){
                c->state=2;
                out = remaining;
            } else {
                out = sizeof(ssbl_dat_reader_buf);
            }
            remaining-=out;
            for(unsigned int i=0;i<out/sizeof(uint64_t);i++){
                ssbl_dat_reader_buf[i] = ssbl_mask^*src++;
            }
            break;
        default:
            out=0;
            break;
    }
    //printf("dat_reader dat=%p, out=%lx\n",dat,out);
    //fflush(stdout);
    return out;
}

static bool ssbl_is_magic(uint64_t dat){
    uint64_t expected_magic;
    memcpy(&expected_magic,SSBL_MAGIC,sizeof(expected_magic));
    return expected_magic == dat;
}

static uint64_t ssbl_op_reduction(uint64_t*op){
    const unsigned int size = sizeof(BN_WORD)*BN_WORDS;
    uint64_t out=0;
    for(unsigned int i=0;i<size/sizeof(uint64_t);i++){
        out ^= op[i];
    }
    //debug_println64x("DEBUG: reduction=0x",out);
    return out;
}

void ssbl_unmask(uint64_t*dat, uint64_t data_size, uint64_t*op, uint64_t mask){
    mask ^= ssbl_op_reduction(op);
    for(uint64_t i=0;i<data_size/sizeof(uint64_t);i++){
        const uint64_t r = *ssbl_mem_readbuf(dat+i);
        const uint64_t w = mask ^ r;
        //if(i==0) {
        //debug_println64x("DEBUG: r=0x",r);
        //debug_println64x("DEBUG: w=0x",w);
        //}
        ssbl_mem_write64(dat+i,w);
    }
}

static int ssbl_main(uint64_t base, uint64_t size, bool call_allowed){
    //debug_println64x("DEBUG: base          0x",base);
    //debug_println64x("DEBUG: size          0x",size);
    uint64_t maska;
    uint64_t maskb;
    rng32_buf(&maska,sizeof(maska));
    uint64_t x_store[BN_WORDS64] = {0};
    uint64_t y_store[BN_WORDS64] = {0};
    rng32_buf(y_store,sizeof(uint64_t));//make x^y unknown
    rng32_buf(&maskb,sizeof(maskb));
    ssbl_mask = maska^maskb;
    
    rsa_x = (BN_WORD*)x_store;
    rsa_y = (BN_WORD*)y_store;
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

    SSBL_PREPARE_LOAD();
    debug_println("INFO: sanity checks passed");

    uint64_t*load_address_ptr = (uint64_t*)(uintptr_t)load_address;

    //store data in internal memory
    uint64_t*dst = load_address_ptr;
    for(uint64_t i=0;i<data_size/sizeof(uint64_t);i++){
        ssbl_mem_write64(dst+i,ssbl_mask^ssbl_read64());
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
    ssbl_write32(status);
    //("DEBUG x\n",x_store,sizeof(x_store)); 
    //debug_println_bytes("DEBUG y\n",y_store,sizeof(y_store)); 
    //debug_println_bytes("DEBUG before xred\n",load_address_ptr,32); 
    ssbl_unmask(load_address_ptr,data_size,x_store,maska);
    //debug_println_bytes("DEBUG after xred\n",load_address_ptr,32);        
    if(status){
        memset(load_address_ptr,0,data_size);
        debug_println("ERROR: sig mismatch, data erased");
        return -1;
    }else{
        if(call_allowed && (start_address+1)){
            SSBL_PREPARE_CALL();
            ssbl_unmask(load_address_ptr,data_size,y_store,maskb);
            //debug_println_bytes("DEBUG RAM code\n",load_address_ptr,32);
            debug_println64x("INFO: sig match, calling 0x",start_address);
            void (*target)(void) = (void (*)(void))(uintptr_t)start_address;
            target();
        } else {
            debug_println("INFO: sig match, data loaded");
        }
        return 0;
    }
}

static int ssbl_main_loop(uint64_t base, uint64_t size, bool call_allowed){
    int status = 0;
    uint32_t errcode;
    if (0 == (errcode = setjmp(ssbl_exception_ctx))){
        while(0<=status){//negative is fatal error
            status = ssbl_main(base, size, call_allowed);
        }
    } else {
        status = errcode;
        debug_println32x("ERROR: Exception caught by ssbl_main_loop 0x",status);
        ssbl_write32(errcode);
    }
    return status;
}