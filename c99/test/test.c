#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "bytes_utils.h"
#include "ssb.h"

//must match input ihex file
//#define BUF_SIZE 0x200
#define BYTES_PER_LINE 16

int main(int argc, char*argv[]){
    char *ihex = "../../test.ihex.signed.ihex";
    uint8_t mem[BUF_SIZE];
    WORD sig[BN_WORDS]={0};
    uint8_t *sig8 = (uint8_t*)&sig;
    WORD computed_sig[BN_WORDS]={0};
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(ihex, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    uint8_t *dat=mem;
    if((dat-mem)==BUF_SIZE){
        dat=sig8;
        printf("get signature\n");
    }else{
        printf("get data\n");
    }
    while ((read = getline(&line, &len, fp)) != -1) {
        if(44==read){
            char *hexstr = line+9;
            hexstr[BYTES_PER_LINE*2]=0;
            //printf("%s\n", hexstr);

            hexstr_to_bytes(dat,BYTES_PER_LINE,hexstr);
            //println_128("",dat);
            dat+=BYTES_PER_LINE;
            if((dat-mem)==BUF_SIZE){
                dat=sig8;
                printf("get signature\n");
            }
        }else{
            //printf("Retrieved line of length %zu:\n", read);
            //printf("%s", line);
        }
    }

    fclose(fp);
    if (line)
        free(line);

    int status = ssb_check_sig(mem,BUF_SIZE,sig);
    if(status){
        printf("ERROR: sig mismatch\n");
    }else{
        printf("sig match\n");
    }
    return status;
}
