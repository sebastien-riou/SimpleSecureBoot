#ifndef __BN_IO_H__
#define __BN_IO_H__

#include "min-bn-math.h"
#include "stdio.h"
static void bn_dumphex(const char*const s, const WORD x[]){
  printf("%s",s);
  for (int i=BN_WORDS-1; i>=0; i--){
    if(BN_WORD_WIDTH > 32){
		printf("%016" PRIX64 ,(uint64_t)x[i]);
	}else if(BN_WORD_WIDTH > 16){
		printf("%08X",(uint32_t)x[i]);
	} else if(BN_WORD_WIDTH > 8){
		printf("%04X",(uint32_t)x[i]);
	} else {
		printf("%02X",(uint32_t)x[i]);
	}
  }
  printf("\n");
}

static void bn_dumpdec(const char*const s, const WORD x[]){
  char res[4096*100];
  char* resptr = &res[4095*100];
  *resptr=0;
  WORD d[BN_WORDS], m[BN_WORDS], t[BN_WORDS], t0[BN_WORDS], t10[BN_WORDS];
  bn_mov0(t0);
  bn_movc(t10,10);
  bn_copy(t,x);
  do{
    bn_div(d,m,t,t10);
    *--resptr = '0'+m[0];
    bn_copy(t,d);
  }
  while (bn_cmp(t0,t)!=0);
  printf("%s%s\n",s,resptr);
}

static void bn_movdec(WORD x[], const char*const s){
  WORD t[BN_WORDS],t10[BN_WORDS],c[BN_WORDS];
  bn_movc(t10,10);
  bn_mov0(x);
  for (WORD i=0; i<strlen(s); i++){
    bn_mul(t,x,t10);
    bn_copy(x,t);
    bn_movc(c,s[i]-'0');
    bn_add(x,c);
  }
}

#endif
