#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "bytes_utils.h"
#include <assert.h>
#include <setjmp.h>

#define SSBL_CUSTOM_IO
uint64_t imagebuf[(BUF_SIZE+32+8+SSB_KEY_LENGTH/8)/8];
static uint64_t ssbl_read64(){
    static unsigned int offset=0;
    return imagebuf[offset++];
}
uint64_t membuf[BUF_SIZE];
static void ssbl_mem_write64(uint64_t*addr, uint64_t dat){
    static unsigned int offset=0;
    static uint64_t base=0;
    uint64_t addr64 = (uint64_t)addr;
    if(0==offset) base = addr64;
    printf("write %016lx at %08lx\n",dat,addr64);
    assert(offset == (addr64-base)/sizeof(uint64_t));
    membuf[offset++] = dat;
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
        if(read>12){
            char *hexstr = line+9;
            hexstr[BYTES_PER_LINE*2]=0;
            //printf("%s\n", hexstr);
            unsigned int nbytes = (read-12)/2;
            hexstr_to_bytes(dat,nbytes,hexstr);
            println_128("",dat);
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

    int status = ssbl_main_loop(0,BUF_SIZE);
    if(status){
        printf("ERROR: sig mismatch\n");
    }else{
        printf("sig match\n");
    }
    return status;
}
