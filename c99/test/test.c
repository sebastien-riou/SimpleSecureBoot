#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "bytes_utils.h"
#include "ssb.h"

//must match input ihex file
//#define BUF_SIZE 0x200
#define BYTES_PER_LINE 16

#include <errno.h>
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    size_t pos;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    c = getc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        if (*lineptr == NULL) {
            return -1;
        }
        *n = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }
            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos ++] = c;
        if (c == '\n') {
            break;
        }
        c = getc(stream);
    }

    (*lineptr)[pos] = '\0';
    return pos;
}

int main(int argc, char*argv[]){
    (void)bytes_utils_remove_unused_warnings;
    if(argc<2) {
        printf("ERROR: need ihex file path as argument\n");
        exit(-1);
    }
    char *ihex = argv[1];//"../../test.ihex.signed.ihex";
    uint8_t mem[BUF_SIZE];
    BN_WORD sig[BN_WORDS]={0};
    uint8_t *sig8 = (uint8_t*)&sig;
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
