#ifndef __MIN_BN_MATH_H__
#define __MIN_BN_MATH_H__

#include <string.h>
#include <stdint.h>

#define BN_WORD_WIDTH	  (sizeof(BN_WORD)*8)
#define BN_BIT            ((BN_SAFE_WIDTH)+(BN_WORD_WIDTH))
#define BN_BYTES          (BN_BIT/8)
#define BN_WORDS          ((BN_BIT)/(BN_WORD_WIDTH))
#define BN_WORDS_MSB_MASK (((BN_WORD)1)<<((BN_WORD_WIDTH)-1))

//x=0
static void bn_mov0(BN_WORD x[]){
	for(unsigned int i=0;i<BN_WORDS;i++){
		x[i]=0;
	}
}

//x=c
static void bn_movc(BN_WORD x[], const BN_WORD c){
	x[0] = c;
	for(unsigned int i=1;i<BN_WORDS;i++){
		x[i]=0;
	}
}

//dest=src
static void bn_copy(BN_WORD dest[], const BN_WORD src[]){
	for(unsigned int i=0;i<BN_WORDS;i++){
		dest[i]=src[i];
	}
}

//x = x<<1;
static void bn_shl1(BN_WORD x[]){
	BN_WORD carry=0;
	for(unsigned int i=0;i<BN_WORDS;i++){
		BN_WORD tmp = (x[i]<<1)| carry;
		carry = (x[i] & BN_WORDS_MSB_MASK) ? 1 : 0;
		x[i] = tmp;
	}
}

//dest+=src
static void bn_add(BN_WORD dest[], const BN_WORD src[]){
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
static void bn_sub(BN_WORD dest[], const BN_WORD src[]){
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
static int bn_cmp(const BN_WORD x[], const BN_WORD y[]){
	for(int i=BN_WORDS-1;i>=0;i--){
		if(x[i]!=y[i]){
			return x[i]>y[i] ? 1 : -1;
		}
	}
	return 0;
}

//(x>>bitn) & 1
static int bn_getbit(const BN_WORD x[], const BN_WORD bitn){
	BN_WORD mask = ((BN_WORD)1) << (bitn % BN_WORD_WIDTH);
	return (x[bitn/BN_WORD_WIDTH] & mask) ? 1 : 0;
}

//position of most significant bit set in x
static int bn_msb(const BN_WORD x[]){
  for (int i=BN_BIT-1; i>=0; i--)
    if (bn_getbit(x,i)!=0) return i;
  return 0;
}

//r=(x*y) mod m
//x,y < m
static void bn_modmul(BN_WORD r[], const BN_WORD x[], const BN_WORD y[], const BN_WORD m[]){
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
static void bn_modexp(BN_WORD x[], const BN_WORD a[], const BN_WORD b[], const BN_WORD m[]){
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

#endif // __MIN_BN_MATH_H__
