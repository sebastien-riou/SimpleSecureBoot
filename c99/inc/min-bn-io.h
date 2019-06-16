#ifndef __BN_IO_H__
#define __BN_IO_H__

#include "min-bn-math.h"
#include "stdio.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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

#endif
