#include "sha256.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void bstr(const unsigned char *data, unsigned long long length){
    for (unsigned long long i = 0; i < length; i++)
		printf("%02X", data[i]);

    printf("\n");
}

int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

int main(int argc, char *argv[]){
    FILE *f;
    if(argc<2) return 0;
    char*fileName = argv[1];
    if ((f = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "Couldn't open <%s> for read\n", fileName);
		return -1;
	}
    size_t mlen = fsize(f);
    //printf("mlen=%lu\n",mlen);
    size_t readcnt=0;
    uint8_t *msg = malloc(mlen);
    if(msg){
        readcnt = fread(msg, 1, mlen, f);
    }
    fclose(f);
    if(0==msg){
        printf("ERROR: could not allocate memory (%lu bytes requested)\n",mlen);
        return -2;
    }
    if(readcnt==mlen){
        uint8_t out[32] = {0};
        sha256_sum(
            msg,     // message,
            mlen,  // mlen,
            out     //digest
        );
        bstr(out, 32);
    }
    free(msg);
    if(readcnt!=mlen) {
        printf("ERROR: could not read file entirely (%lu bytes requested, %lu bytes actually read)\n",mlen,readcnt);
        return -3;
    }
    return 0;
}
