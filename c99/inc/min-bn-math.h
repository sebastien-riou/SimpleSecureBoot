#ifndef __MIN_BN_MATH_H__
#define __MIN_BN_MATH_H__

#include <string.h>
#include <stdint.h>

#ifndef BN_FUNC
#define BN_FUNC static
#endif

#define BN_WORD_WIDTH	  (sizeof(BN_WORD)*8)
#define BN_BITS           ((BN_SAFE_WIDTH)+(BN_WORD_WIDTH))
#define BN_BYTES          (BN_BITS/8)
#define BN_WORDS          ((BN_BITS)/(BN_WORD_WIDTH))
#define BN_WORDS_MSB_MASK (((BN_WORD)1)<<((BN_WORD_WIDTH)-1))
#define BN_WORDS_LSB_MASK 1

#ifdef BN_MATH_DEBUG
#include "min-bn-io.h"
#endif

//x=0
BN_FUNC void bn_mov0(BN_WORD x[]){
	for(unsigned int i=0;i<BN_WORDS;i++){
		x[i]=0;
	}
}

//x=c
BN_FUNC void bn_movc(BN_WORD x[], const BN_WORD c){
	x[0] = c;
	for(unsigned int i=1;i<BN_WORDS;i++){
		x[i]=0;
	}
}

//dest=src
BN_FUNC void bn_copy(BN_WORD dest[], const BN_WORD src[]){
	for(unsigned int i=0;i<BN_WORDS;i++){
		dest[i]=src[i];
	}
}

//x = x<<1;
BN_FUNC void bn_shl1(BN_WORD x[]){
	BN_WORD carry=0;
	for(unsigned int i=0;i<BN_WORDS;i++){
		BN_WORD tmp = (x[i]<<1)| carry;
		carry = (x[i] & BN_WORDS_MSB_MASK) ? BN_WORDS_LSB_MASK : 0;
		x[i] = tmp;
	}
}

//x = x>>1;
BN_FUNC void bn_shr1(BN_WORD x[]){
	BN_WORD carry=0;
	for(int i=BN_WORDS-1;i>=0;i--){
		BN_WORD tmp = (x[i]>>1)| carry;
		carry = (x[i] & BN_WORDS_LSB_MASK) ? BN_WORDS_MSB_MASK : 0;
		x[i] = tmp;
	}
}

//dest+=src
BN_FUNC void bn_add(BN_WORD dest[], const BN_WORD src[]){
	BN_WORD carry=0;
	for(unsigned int i=0;i<BN_WORDS;i++){
		BN_WORD tmp = dest[i] + carry;
		if(tmp<dest[i]) carry = 1;
		else carry = 0;
		dest[i] = tmp;
		tmp = dest[i] + src[i];
		if(tmp<dest[i]) carry |= 1;
		else if(tmp<src[i]) carry |= 1;
		dest[i] = tmp;
	}
}

//dest-=src
BN_FUNC void bn_sub(BN_WORD dest[], const BN_WORD src[]){
	BN_WORD carry = 1;
	for(unsigned int i=0;i<BN_WORDS;i++){
		const BN_WORD tmp1 = dest[i] + carry;
		if(tmp1<dest[i]) carry = 1;
		else carry = 0;
		const BN_WORD srci = ~src[i];
		const BN_WORD tmp2 = tmp1 + srci;
		if(tmp2<tmp1) carry |= 1;
		else if(tmp2<srci) carry |= 1;
		dest[i] = tmp2;
	}
}

//1 if x>y
//0 if x==y
//-1 if x<y
BN_FUNC int bn_cmp(const BN_WORD x[], const BN_WORD y[]){
	for(int i=BN_WORDS-1;i>=0;i--){
		if(x[i]!=y[i]){
			return x[i]>y[i] ? 1 : -1;
		}
	}
	return 0;
}

//(x>>bitn) & 1
BN_FUNC int bn_getbit(const BN_WORD x[], const BN_WORD bitn){
	BN_WORD mask = ((BN_WORD)1) << (bitn % BN_WORD_WIDTH);
	return (x[bitn/BN_WORD_WIDTH] & mask) ? 1 : 0;
}

//position of most significant bit set in x
BN_FUNC int bn_msb(const BN_WORD x[]){
  for (int i=BN_BITS-1; i>=0; i--)
    if (bn_getbit(x,i)!=0) return i;
  return 0;
}

//r=(x*y) mod m
//x,y < m
BN_FUNC void bn_modmul(BN_WORD r[], const BN_WORD x[], const BN_WORD y[], const BN_WORD m[]){
  bn_mov0(r);
  for (int i=bn_msb(y); i>=0; i--){
    bn_shl1(r);
    if (bn_cmp(r,m)>=0) bn_sub(r,m);
    if (bn_getbit(y,i)){
      bn_add(r,x);
      if (bn_cmp(r,m)>=0) bn_sub(r,m);
    }
  }
}

//x=(a power b) mod m
//a,b < m
BN_FUNC void bn_modexp(BN_WORD x[], const BN_WORD a[], const BN_WORD b[], const BN_WORD m[]){
  bn_movc(x,1);
  BN_WORD p[BN_WORDS], t[BN_WORDS];
  bn_copy(p,a);
  int msb=bn_msb(b);
  for (int i=0; i<=msb; i++){
    if (bn_getbit(b,i)){
      bn_modmul(t,x,p,m);
      bn_copy(x,t);
    }
    bn_modmul(t,p,p,m);
    bn_copy(p,t);
  }
}

//A=(x * y * R^-1) mod m
//x,y < m
BN_FUNC void bn_modmul_montgommery(BN_WORD A[], const BN_WORD x[], const BN_WORD y[], const BN_WORD m[], unsigned int mp){
  bn_movc(A,0);
  int msb=bn_msb(m);
  unsigned int y0=y[0] & 1;
  for (int i=0; i<=msb; i++){
    unsigned int a0=A[0] & 1;
    unsigned int xi = bn_getbit(x,i);
    unsigned int ui = (a0 ^ (xi & y0)) & mp;
    if (xi){
      bn_add(A,y);
    }
    if (ui){
      bn_add(A,m);
    }
    bn_shr1(A);
  }
  if (bn_cmp(A,m)>=0) bn_sub(A,m);
}

BN_FUNC void bn_swap_pointers(BN_WORD**A,BN_WORD**B){
    intptr_t *a=(intptr_t*)A;
    intptr_t *b=(intptr_t*)B;
    *a^=*b;
    *b^=*a;
    *a^=*b;
}

//out=(x power y) mod m
//x,y < m
BN_FUNC void bn_modexp_montgommery(BN_WORD out[], const BN_WORD x[], const BN_WORD y[], const BN_WORD m[], const BN_WORD R[], const BN_WORD R2[], unsigned int mp){
  BN_WORD xp[BN_WORDS],t[BN_WORDS];
  BN_WORD *A=out;
  BN_WORD *next_A=t;
  bn_modmul_montgommery(xp,x,R2,m,mp);
  bn_copy(A,R);
  int msb=bn_msb(y);
  for (int i=msb; i>=0; i--){
    bn_modmul_montgommery(next_A,A,A,m,mp);
    if (bn_getbit(y,i)){
      bn_modmul_montgommery(A,next_A,xp,m,mp);
    }else{
      bn_swap_pointers(&A,&next_A);
    }
  }
  bn_movc(xp,1);
  bn_modmul_montgommery(next_A,A,xp,m,mp);
  if(out!=next_A){
      bn_copy(out,next_A);
  }
}

static void bn_remove_unused_warnings(void){
    (void)bn_modexp;
    (void)bn_modexp_montgommery;
}

#endif // __MIN_BN_MATH_H__
