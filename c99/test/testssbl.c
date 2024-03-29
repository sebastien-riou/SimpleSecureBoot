#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "bytes_utils.h"
#include <assert.h>
#include <setjmp.h>

void rng32_buf(void*dst,unsigned int size){
    uint8_t*dst8 = (uint8_t*)dst;
    for(unsigned int i=0;i<size;i++){
        dst8[i] = rand();
    }
}

#define SSBL_CUSTOM_IO
uint64_t imagebuf[(BUF_SIZE+32+8+SSB_KEY_LENGTH/8)/8];
static uint64_t ssbl_read64(){
    static unsigned int offset=0;
    return imagebuf[offset++];
}
static void ssbl_write32(uint32_t dat){
    printf("SSBL WRITE32: 0x%08x\n",dat);
}
uint64_t membuf[BUF_SIZE];
static uint64_t membuf_base=0;
static void ssbl_mem_write64(uint64_t*addr, uint64_t dat){
    static unsigned int ssbl_mem_write64_offset=0;
    uint64_t addr64 = (uint64_t)addr;
    if(0==ssbl_mem_write64_offset) membuf_base = addr64;
    if(membuf_base == addr64) ssbl_mem_write64_offset = 0;
    //printf("write %016lx at %08lx\n",dat,addr64);
    assert(ssbl_mem_write64_offset == (addr64-membuf_base)/sizeof(uint64_t));
    membuf[ssbl_mem_write64_offset++] = dat;
}
static const uint64_t* const ssbl_mem_readbuf(const uint64_t*const addr){
    uint64_t addr64 = (uint64_t)addr;
    unsigned int offset = (addr64-membuf_base)/sizeof(uint64_t);
    return membuf+offset;
}
#define PRINT_PREFIX debug_
static void debug_print_impl(const char*msg){
	printf("%s",msg);
}
#include "print.h"

#define PRINT_PREFIX debug_
#include "assert_print.h"
//#define assert(__e) ASSERT_TRUE(__e)

static jmp_buf ssbl_exception_ctx;

#include "ssbl.h"

//must match input ihex file
//#define BUF_SIZE 0x200
#define BYTES_PER_LINE 16

#include <errno.h>
#include "test.h"

int main(int argc, char*argv[]){
    (void)bytes_utils_remove_unused_warnings;
    (void)default_dat_reader;


    if(argc<2) {
        printf("ERROR: need ihex file path as argument\n");
        exit(-1);
    }
    char *ihex = argv[1];//"../../test.ihex.ssbl.ihex";
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(ihex, "r");
    if (fp == NULL){
        printf("ERROR: cannot open file %s\n",ihex);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    uint8_t *dat=(uint8_t*)imagebuf;
    printf("get data\n");
    while ((read = getline(&line, &len, fp)) != -1) {
        if(read>12 && line[8]=='0'){
            char *hexstr = line+9;
            hexstr[BYTES_PER_LINE*2]=0;
            //printf("%s\n", hexstr);
            unsigned int nbytes = (read-12)/2;
            hexstr_to_bytes(dat,nbytes,hexstr);
            //if(dat-(uint8_t*)imagebuf < 128) println_128("",dat);
            assert(dat-(uint8_t*)imagebuf + nbytes <= sizeof(imagebuf));
            dat+=nbytes;
        }else{
            //printf("Retrieved line of length %zu:\n", read);
            //printf("%s", line);
        }
    }

    fclose(fp);
    if (line)
        free(line);

    int status = ssbl_main(0,1UL<<63,0); //BUF_SIZE);
    if(status){
        printf("ERROR: sig mismatch\n");
    }else{
        printf("sig match\n");
    }
    return status;
}
