#ifndef __MIN_BN_MATH_H__
#define __MIN_BN_MATH_H__

#include <string.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifndef MIN_BN_MATH_TEST
	#define MIN_BN_MATH_TEST 0
#endif

#if MIN_BN_MATH_TEST
	//only needed for self test builds
	#include <stdio.h>
	#include <stdlib.h>
	#define BN_SAFE_WIDTH 56
	typedef uint8_t WORD;

	//generic helpers used in test functions
	#define NUM(a) (sizeof(a) / sizeof(*a))
	#define STR_EXPAND(tok) #tok
	#define STR(tok) STR_EXPAND(tok)
	#define DBG_PRINT32(a) printf("%s = %08X\n",STR(a),a);
	#define DBG_PRINT64(a) printf("%s = %016" PRIX64 "\n",STR(a),a);

	#define ASSERT_EQ(actual,expected) do{\
		if((actual)!=(expected)){\
			printf("%s != %s\n",STR(actual),STR(expected));\
			printf("actual value is   0x%08X\n",actual);\
			printf("expected value is 0x%08X\n",expected);\
			exit(-1);\
		}\
	}while(0)

    #define ASSERT_EQ64(actual,expected) do{\
		if((actual)!=(expected)){\
			printf("%s != %s\n",STR(actual),STR(expected));\
			printf("actual value is   0x%016" PRIX64 "\n",actual);\
			printf("expected value is 0x%016" PRIX64 "\n",expected);\
			exit(-1);\
		}\
	}while(0)
#endif

#define BN_WORD_WIDTH	  (sizeof(WORD)*8)
#define BN_BIT            ((BN_SAFE_WIDTH)+(BN_WORD_WIDTH))
#define BN_BYTES          (BN_BIT/8)
#define BN_WORDS          ((BN_BIT)/(BN_WORD_WIDTH))
#define BN_WORDS_MSB_MASK (((WORD)1)<<((BN_WORD_WIDTH)-1))

//x=0
static void bn_mov0(WORD x[]){
	for(unsigned int i=0;i<BN_WORDS;i++){
		x[i]=0;
	}
}

//x=c
static void bn_movc(WORD x[], const WORD c){
	x[0] = c;
	for(unsigned int i=1;i<BN_WORDS;i++){
		x[i]=0;
	}
}

//dest=src
static void bn_copy(WORD dest[], const WORD src[]){
	for(unsigned int i=0;i<BN_WORDS;i++){
		dest[i]=src[i];
	}
}

//x = x<<1;
static void bn_shl1(WORD x[]){
	WORD carry=0;
	for(unsigned int i=0;i<BN_WORDS;i++){
		WORD tmp = (x[i]<<1)| carry;
		carry = (x[i] & BN_WORDS_MSB_MASK) ? 1 : 0;
		x[i] = tmp;
	}
}

//dest+=src
static void bn_add(WORD dest[], const WORD src[]){
	WORD carry=0;
	for(unsigned int i=0;i<BN_WORDS;i++){
		WORD tmp = dest[i] + carry;
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
static void bn_sub(WORD dest[], const WORD src[]){
	WORD carry = 1;
	for(unsigned int i=0;i<BN_WORDS;i++){
		const WORD tmp1 = dest[i] + carry;
		if(tmp1<dest[i]) carry = 1;
		else carry = 0;
		const WORD srci = ~src[i];
		const WORD tmp2 = tmp1 + srci;
		if(tmp2<tmp1) carry |= 1;
		else if(tmp2<srci) carry |= 1;
		dest[i] = tmp2;
	}
}

//1 if x>y
//0 if x==y
//-1 if x<y
static int bn_cmp(const WORD x[], const WORD y[]){
	for(int i=BN_WORDS-1;i>=0;i--){
		if(x[i]!=y[i]){
			return x[i]>y[i] ? 1 : -1;
		}
	}
	return 0;
}

//(x>>bitn) & 1
static int bn_getbit(const WORD x[], const WORD bitn){
	WORD mask = ((WORD)1) << (bitn % BN_WORD_WIDTH);
	return (x[bitn/BN_WORD_WIDTH] & mask) ? 1 : 0;
}

//position of most significant bit set in x
static int bn_msb(const WORD x[]){
  for (int i=BN_BIT-1; i>=0; i--)
    if (bn_getbit(x,i)!=0) return i;
  return 0;
}

//r=x*y
static void bn_mul(WORD r[], const WORD x[], const WORD y[]){
  bn_mov0(r);
  for (int i=bn_msb(y); i>=0; i--){
    bn_shl1(r);
    if (bn_getbit(y,i)) bn_add(r,x);
  }
}

//m=x mod y
static void bn_mod(WORD m[], const WORD x[], const WORD y[]){
  bn_mov0(m);
  for (int i=bn_msb(x); i>=0; i--){
    bn_shl1(m);
    m[0]|=bn_getbit(x,i);
    if (bn_cmp(m,y)>=0) bn_sub(m,y);
  }
}

//given x and y, compute d and r such that x=y*d+r
static void bn_div(WORD d[], WORD r[], const WORD x[], const WORD y[]){
  bn_mov0(d);
  bn_mov0(r);
  int msb=bn_msb(x);
  for (int i=msb; i>=0; i--){
    bn_shl1(d);
    bn_shl1(r);
    r[0]|=bn_getbit(x,i);
    if (bn_cmp(r,y)>=0){
      bn_sub(r,y);
      d[0]|=1;
    }
  }
}

//r=(x*y) mod m
//x,y < m
static void bn_modmul(WORD r[], const WORD x[], const WORD y[], const WORD m[]){
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
static void bn_modexp(WORD x[], const WORD a[], const WORD b[], const WORD m[]){
  bn_movc(x,1);
  WORD p[BN_WORDS], t[BN_WORDS];
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

#if MIN_BN_MATH_TEST

static void test_bn_mov0(void){
	uint64_t test_dat64[3];
	memset(test_dat64,0xFF,sizeof(test_dat64));
	WORD *test_dat=(WORD*) &(test_dat64[1]);
	ASSERT_EQ64(test_dat64[0],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[1],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[2],0xFFFFFFFFFFFFFFFF);
	bn_mov0(test_dat);
	ASSERT_EQ64(test_dat64[0],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[1],0x0000000000000000UL);
	ASSERT_EQ64(test_dat64[2],0xFFFFFFFFFFFFFFFF);
	printf("test_bn_mov0 pass\n");
}

static void test_bn_movc(void){
	uint64_t test_dat64[3];
	memset(test_dat64,0xFF,sizeof(test_dat64));
	WORD *test_dat=(WORD*) &(test_dat64[1]);
	ASSERT_EQ64(test_dat64[0],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[1],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[2],0xFFFFFFFFFFFFFFFF);
	bn_movc(test_dat,0xAA);
	ASSERT_EQ64(test_dat64[0],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[1],0x00000000000000AAUL);
	ASSERT_EQ64(test_dat64[2],0xFFFFFFFFFFFFFFFF);
	printf("test_bn_movc pass\n");
}

static void test_bn_copy(void){
	uint64_t test_dat64[3];
	memset(test_dat64,0xFF,sizeof(test_dat64));
	WORD *test_dat=(WORD*) &(test_dat64[1]);
	ASSERT_EQ64(test_dat64[0],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[1],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[2],0xFFFFFFFFFFFFFFFF);
	uint64_t val64 = 0xa4093822299f31d0;
	WORD *val=(WORD*) &(val64);
	bn_copy(test_dat,val);
	ASSERT_EQ64(test_dat64[0],0xFFFFFFFFFFFFFFFF);
	ASSERT_EQ64(test_dat64[1],0xa4093822299f31d0);
	ASSERT_EQ64(test_dat64[2],0xFFFFFFFFFFFFFFFF);
	printf("test_bn_copy pass\n");
}

static void bn_shl1_ref64(uint64_t *x){
	*x = (*x)<<1;
}

static void test_bn_shl1(void){
	uint64_t tv[] = {
		0x0000000000000000UL,
		0x13198a2e03707344,
		0xa4093822299f31d0,
		0x082efa98ec4e6c89,
		0x452821e638d01377,
		0xbe5466cf34e90c6c,
		0x7ef84f78fd955cb1,
		0x85840851f1ac43aa,
		0xc882d32f25323c54,
		0x64a51195e0e3610d,
		0xd3b5a399ca0c2399,
		0xc0ac29b7c97c50dd
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t test_dat64[3];
		WORD *test_dat=(WORD*) &(test_dat64[1]);
		test_dat64[0] = 0xD6DCB5978DE756ED;test_dat64[2] = 0x892F599F46761CD3;
		test_dat64[1] = tv[i];
		bn_shl1(test_dat);
		bn_shl1_ref64(&(tv[i]));
		ASSERT_EQ64(test_dat64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(test_dat64[2],0x892F599F46761CD3);
		ASSERT_EQ64(test_dat64[1],tv[i]);
	}
	printf("test_bn_shl1 pass\n");
}

static void bn_add_ref64(uint64_t *dest, uint64_t *src){
	*dest+=*src;
}

static void test_bn_add(void){
	uint64_t tv[][2] = {
		{0x0000000000000000UL,0x0000000000000000UL},
		{0x18319a2e03707344UL,0x07313198a3742e04UL},
		{0xa3409822299f31d0UL,0xf31a4093899d2220UL},
		{0x0f82ea98ec4e6c89UL,0xe6c082efac4898e9UL},
		{0x425281e638d01377UL,0x0134528218d7e637UL},
		{0xb6e546cf34e90c6cUL,0x90cbe54664e6cf3cUL},
		{0x74ef8f78fd955cb1UL,0x55c7ef84fd9b78f1UL},
		{0x80584851f1ac43aaUL,0xc438584081aa51faUL},
		{0xcd88232f25323c54UL,0x23cc882d35352f24UL},
		{0x614a5195e0e3610dUL,0x36164a5110e095edUL},
		{0xda3b5399ca0c2399UL,0xc23d3b5a3a0999c9UL},
		{0xc20ac9b7c97c50ddUL,0xc50c0ac2997db7cdUL},
		{0xffffffffffffffffUL,0xffffffffffffffffUL},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		bn_add(a,b);
		bn_add_ref64(&(tv[i][0]),&(tv[i][1]));
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
	}
	printf("test_bn_add pass\n");
}

static void bn_sub_ref64(uint64_t *dest, uint64_t *src){
	*dest-=*src;
}

static void test_bn_sub(void){
	uint64_t tv[][2] = {
		{0x0000000000000000UL,0x0000000000000000UL},
		{0x18319a2e03707344UL,0x07313198a3742e04UL},
		{0xa3409822299f31d0UL,0xf31a4093899d2220UL},
		{0x0f82ea98ec4e6c89UL,0xe6c082efac4898e9UL},
		{0x425281e638d01377UL,0x0134528218d7e637UL},
		{0xb6e546cf34e90c6cUL,0x90cbe54664e6cf3cUL},
		{0x74ef8f78fd955cb1UL,0x55c7ef84fd9b78f1UL},
		{0x80584851f1ac43aaUL,0xc438584081aa51faUL},
		{0xcd88232f25323c54UL,0x23cc882d35352f24UL},
		{0x614a5195e0e3610dUL,0x36164a5110e095edUL},
		{0xda3b5399ca0c2399UL,0xc23d3b5a3a0999c9UL},
		{0xc20ac9b7c97c50ddUL,0xc50c0ac2997db7cdUL},
		{0xffffffffffffffffUL,0xffffffffffffffffUL},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		bn_sub(a,b);
		bn_sub_ref64(&(tv[i][0]),&(tv[i][1]));
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
	}
	printf("test_bn_sub pass\n");
}

static int bn_cmp_ref64(uint64_t *a, uint64_t *b){
	return *a==*b ? 0 :
			*a>*b ? 1 : -1;
}

static void test_bn_cmp(void){
	uint64_t tv[][2] = {
		{0x0000000000000000UL,0x0000000000000000UL},
		{0x18319a2e03707344UL,0x07313198a3742e04UL},
		{0xa3409822299f31d0UL,0xf31a4093899d2220UL},
		{0x0f82ea98ec4e6c89UL,0xe6c082efac4898e9UL},
		{0x425281e638d01377UL,0x0134528218d7e637UL},
		{0xb6e546cf34e90c6cUL,0x90cbe54664e6cf3cUL},
		{0x74ef8f78fd955cb1UL,0x55c7ef84fd9b78f1UL},
		{0x80584851f1ac43aaUL,0xc438584081aa51faUL},
		{0xcd88232f25323c54UL,0x23cc882d35352f24UL},
		{0x614a5195e0e3610dUL,0x36164a5110e095edUL},
		{0xda3b5399ca0c2399UL,0xc23d3b5a3a0999c9UL},
		{0xc20ac9b7c97c50ddUL,0xc50c0ac2997db7cdUL},
		{0xffffffffffffffffUL,0xffffffffffffffffUL},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		const int res = bn_cmp(a,b);
		const int res_ref = bn_cmp_ref64(&(tv[i][0]),&(tv[i][1]));
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ(res,res_ref);
	}
	printf("test_bn_cmp pass\n");
}

//bn_getbit covered by test_bn_msb, test_bn_mul and test_bn_mod

static void test_bn_msb(void){
	for(int i=0;i<64;i++){
		uint64_t a64[3];
		WORD *a=(WORD*) &(a64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		const uint64_t lsbs = (((uint64_t)0x750c0ac2997db7cd) >> (63-i));
		const uint64_t tv = (((uint64_t)1)<<i) | lsbs;
		a64[1] = tv;
		const int res = bn_msb(a);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv);
		ASSERT_EQ(res,i);
	}
	printf("test_bn_msb pass\n");
}

static void bn_mul_ref64(uint64_t *r, uint64_t x, uint64_t y){
	*r=x*y;
}

static void test_bn_mul(void){
	uint64_t tv[][2] = {
		{0x0000000000000000UL,0x0000000000000000UL},
		{0x18319a2e03707344UL,0x07313198a3742e04UL},
		{0xa3409822299f31d0UL,0xf31a4093899d2220UL},
		{0x0f82ea98ec4e6c89UL,0xe6c082efac4898e9UL},
		{0x425281e638d01377UL,0x0134528218d7e637UL},
		{0xb6e546cf34e90c6cUL,0x90cbe54664e6cf3cUL},
		{0x74ef8f78fd955cb1UL,0x55c7ef84fd9b78f1UL},
		{0x80584851f1ac43aaUL,0xc438584081aa51faUL},
		{0xcd88232f25323c54UL,0x23cc882d35352f24UL},
		{0x614a5195e0e3610dUL,0x36164a5110e095edUL},
		{0xda3b5399ca0c2399UL,0xc23d3b5a3a0999c9UL},
		{0xc20ac9b7c97c50ddUL,0xc50c0ac2997db7cdUL},
		{0xffffffffffffffffUL,0xffffffffffffffffUL},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		uint64_t c64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		WORD *c=(WORD*) &(c64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		c64[0] = 0xD6DCB5978DE756ED;c64[2] = 0x892F599F46761CD3;
		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		uint64_t res_ref;
		bn_mul(c,a,b);
		bn_mul_ref64(&res_ref,tv[i][0],tv[i][1]);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ64(c64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(c64[2],0x892F599F46761CD3);
		ASSERT_EQ64(c64[1],res_ref);
	}
	printf("test_bn_mul pass\n");
}

static void bn_mod_ref64(uint64_t *m, uint64_t x, uint64_t y){
	if(0==y) {printf("0==y\n");exit(-1);}
	*m=x%y;
}

static void test_bn_mod(void){
	uint64_t tv[][2] = {
		{0x18319a2e03707344,0x07313198a3742e04},
		{0xa3409822299f31d0,0xf31a4093899d2220},
		{0x0f82ea98ec4e6c89,0xe6c082efac4898e9},
		{0x425281e638d01377,0x0134528218d7e637},
		{0xb6e546cf34e90c6c,0x90cbe54664e6cf3c},
		{0x74ef8f78fd955cb1,0x55c7ef84fd9b78f1},
		{0x80584851f1ac43aa,0xc438584081aa51fa},
		{0xcd88232f25323c54,0x23cc882d35352f24},
		{0x614a5195e0e3610d,0x36164a5110e095ed},
		{0xda3b5399ca0c2399,0xc23d3b5a3a0999c9},
		{0xc20ac9b7c97c50dd,0xc50c0ac2997db7cd},
		{0xffffffffffffffff,0xffffffffffffffff},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		uint64_t c64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		WORD *c=(WORD*) &(c64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		c64[0] = 0xD6DCB5978DE756ED;c64[2] = 0x892F599F46761CD3;
		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		uint64_t res_ref;
		bn_mod(c,a,b);
		bn_mod_ref64(&res_ref,tv[i][0],tv[i][1]);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ64(c64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(c64[2],0x892F599F46761CD3);
		ASSERT_EQ64(c64[1],res_ref);
	}
	printf("test_bn_mod pass\n");
}

uint64_t mul_mod_ref63(uint64_t a, uint64_t b, uint64_t m){
   uint64_t d = 0, mp2 = m >> 1;
   int i;
   if (a >= m) a %= m;
   if (b >= m) b %= m;
   for (i = 0; i < 64; ++i)
   {
       d = (d > mp2) ? (d << 1) - m : d << 1;
       if (a & 0x8000000000000000ULL)
           d += b;
       if (d > m) d -= m;
       a <<= 1;
   }
   return d;
}

static void bn_modmul_ref64(uint64_t *r, uint64_t x, uint64_t y, uint64_t m){
	if(0==m) {printf("0==m\n");exit(-1);}
	if(x>=m) {printf("x>=m\n");exit(-1);}
	if(y>=m) {printf("y>=m\n");exit(-1);}
	if(m>>63) {printf("m>>63 != 0\n");exit(-1);}
	*r = mul_mod_ref63(x, y, m);
}

static void bn_modmul_ref32(uint64_t *r, uint64_t x, uint64_t y, uint64_t m){
	if(0==m) {printf("0==m\n");exit(-1);}
	if(x>=m) {printf("x>=m\n");exit(-1);}
	if(y>=m) {printf("y>=m\n");exit(-1);}
	if(m>>32) {printf("m>>32 != 0\n");exit(-1);}
	*r = (x*y)%m;
}

static void test_bn_modmul(void){
	uint64_t tv[][3] = {
		{0x18319a2e03707344,0x07313198a3742e04,0x008731e31a372048},
		{0xa3409822299f31d0,0xf31a4093899d2220,0xdf331a2408992209},
		{0x0f82ea98ec4e6c89,0xe6c082efac4898e9,0x8ef6c0882ac49e9e},
		{0x425281e638d01377,0x0134528218d7e637,0x70213465218de378},
		{0xb6e546cf34e90c6c,0x90cbe54664e6cf3c,0x6960cbfe564ec3c4},
		{0x74ef8f78fd955cb1,0x55c7ef84fd9b78f1,0xb545c78effd97f18},
		{0x80584851f1ac43aa,0xc438584081aa51fa,0xac043815881a5fa4},
		{0xcd88232f25323c54,0x23cc882d35352f24,0x52d3ccf883532242},
		{0x614a5195e0e3610d,0x36164a5110e095ed,0x03161654a10e9ed5},
		{0xda3b5399ca0c2399,0xc23d3b5a3a0999c9,0x9ca23d93b3a09c95},
		{0xc20ac9b7c97c50dd,0xc50c0ac2997db7cd,0xdc250c70a997bcdc},
		{0xfffffffffffffffe,0xfffffffffffffffe,0xffffffffffffffff},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		uint64_t c64[3];
		uint64_t d64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		WORD *c=(WORD*) &(c64[1]);
		WORD *d=(WORD*) &(d64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		c64[0] = 0xD6DCB5978DE756ED;c64[2] = 0x892F599F46761CD3;
		d64[0] = 0xD6DCB5978DE756ED;d64[2] = 0x892F599F46761CD3;

		//the reference function support only 63 bit numbers
		tv[i][0] >>= 1;
		tv[i][1] >>= 1;
		tv[i][2] >>= 1;

		//our implementation requires all input are modulo m
		tv[i][0] = (tv[i][0])%(tv[i][2]);
		tv[i][1] = (tv[i][1])%(tv[i][2]);

		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		c64[1] = tv[i][2];
		uint64_t res_ref;
		bn_modmul(d,a,b,c);
		bn_modmul_ref64(&res_ref,tv[i][0],tv[i][1],tv[i][2]);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ64(c64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(c64[2],0x892F599F46761CD3);
		ASSERT_EQ64(c64[1],tv[i][2]);
		ASSERT_EQ64(d64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(d64[2],0x892F599F46761CD3);
		ASSERT_EQ64(d64[1],res_ref);


		//the second reference function support only 32 bit numbers
		tv[i][0] >>= 31;
		tv[i][1] >>= 31;
		tv[i][2] >>= 31;

		//our implementation requires all input are modulo m
		tv[i][0] = (tv[i][0])%(tv[i][2]);
		tv[i][1] = (tv[i][1])%(tv[i][2]);

		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		c64[1] = tv[i][2];
		bn_modmul(d,a,b,c);
		bn_modmul_ref32(&res_ref,tv[i][0],tv[i][1],tv[i][2]);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ64(c64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(c64[2],0x892F599F46761CD3);
		ASSERT_EQ64(c64[1],tv[i][2]);
		ASSERT_EQ64(d64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(d64[2],0x892F599F46761CD3);
		ASSERT_EQ64(d64[1],res_ref);
	}
	printf("test_bn_modmul pass\n");
}

static void bn_div_ref32(uint64_t *r, uint64_t *m, uint64_t x, uint64_t y){
	if(0==y) {printf("0==y\n");exit(-1);}
	*r = x/y;
	*m = x%y;
}

static void test_bn_div(void){
	uint64_t tv[][3] = {
		{0x18319a2e03707344,0x07313198a3742e04},
		{0xa3409822299f31d0,0xf31a4093899d2220},
		{0x0f82ea98ec4e6c89,0xe6c082efac4898e9},
		{0x425281e638d01377,0x0134528218d7e637},
		{0xb6e546cf34e90c6c,0x90cbe54664e6cf3c},
		{0x74ef8f78fd955cb1,0x55c7ef84fd9b78f1},
		{0x80584851f1ac43aa,0xc438584081aa51fa},
		{0xcd88232f25323c54,0x23cc882d35352f24},
		{0x614a5195e0e3610d,0x36164a5110e095ed},
		{0xda3b5399ca0c2399,0xc23d3b5a3a0999c9},
		{0xc20ac9b7c97c50dd,0xc50c0ac2997db7cd},
		{0xfffffffffffffffe,0xfffffffffffffffe},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		uint64_t c64[3];
		uint64_t d64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		WORD *c=(WORD*) &(c64[1]);
		WORD *d=(WORD*) &(d64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		c64[0] = 0xD6DCB5978DE756ED;c64[2] = 0x892F599F46761CD3;
		d64[0] = 0xD6DCB5978DE756ED;d64[2] = 0x892F599F46761CD3;

		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		uint64_t res_ref;
		uint64_t m_ref;

		//the second reference function support only 32 bit numbers
		tv[i][0] >>= 32;
		tv[i][1] >>= 32;

		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		bn_div(d,c,a,b);
		bn_div_ref32(&res_ref,&m_ref,tv[i][0],tv[i][1]);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ64(c64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(c64[2],0x892F599F46761CD3);
		ASSERT_EQ64(c64[1],m_ref);
		ASSERT_EQ64(d64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(d64[2],0x892F599F46761CD3);
		ASSERT_EQ64(d64[1],res_ref);
	}
	printf("test_bn_div pass\n");
}

static uint64_t pow_mod_ref63(uint64_t a, uint64_t b, uint64_t m){
    uint64_t r = m==1?0:1;
    while (b > 0) {
        if(b & 1)
            r = mul_mod_ref63(r, a, m);
        b = b >> 1;
        a = mul_mod_ref63(a, a, m);
    }
    return r;
}

static void bn_modexp_ref64(uint64_t *r, uint64_t x, uint64_t y, uint64_t m){
	if(x>=m) {printf("x>=m\n");exit(-1);}
	if(y>=m) {printf("y>=m\n");exit(-1);}
	if(m>>63) {printf("m>>63 != 0\n");exit(-1);}
	*r = pow_mod_ref63(x, y, m);
}

static void test_bn_modexp(void){
	uint64_t tv[][3] = {
		{0x18319a2e03707344,0x07313198a3742e04,0x008731e31a372048},
		{0xa3409822299f31d0,0xf31a4093899d2220,0xdf331a2408992209},
		{0x0f82ea98ec4e6c89,0xe6c082efac4898e9,0x8ef6c0882ac49e9e},
		{0x425281e638d01377,0x0134528218d7e637,0x70213465218de378},
		{0xb6e546cf34e90c6c,0x90cbe54664e6cf3c,0x6960cbfe564ec3c4},
		{0x74ef8f78fd955cb1,0x55c7ef84fd9b78f1,0xb545c78effd97f18},
		{0x80584851f1ac43aa,0xc438584081aa51fa,0xac043815881a5fa4},
		{0xcd88232f25323c54,0x23cc882d35352f24,0x52d3ccf883532242},
		{0x614a5195e0e3610d,0x36164a5110e095ed,0x03161654a10e9ed5},
		{0xda3b5399ca0c2399,0xc23d3b5a3a0999c9,0x9ca23d93b3a09c95},
		{0xc20ac9b7c97c50dd,0xc50c0ac2997db7cd,0xdc250c70a997bcdc},
		{0xfffffffffffffffe,0xfffffffffffffffe,0xffffffffffffffff},
	};
	for(int i=0;i<NUM(tv);i++){
		uint64_t a64[3];
		uint64_t b64[3];
		uint64_t c64[3];
		uint64_t d64[3];
		WORD *a=(WORD*) &(a64[1]);
		WORD *b=(WORD*) &(b64[1]);
		WORD *c=(WORD*) &(c64[1]);
		WORD *d=(WORD*) &(d64[1]);
		a64[0] = 0xD6DCB5978DE756ED;a64[2] = 0x892F599F46761CD3;
		b64[0] = 0xD6DCB5978DE756ED;b64[2] = 0x892F599F46761CD3;
		c64[0] = 0xD6DCB5978DE756ED;c64[2] = 0x892F599F46761CD3;
		d64[0] = 0xD6DCB5978DE756ED;d64[2] = 0x892F599F46761CD3;

		//the reference function support only 63 bit numbers
		tv[i][0] >>= 1;
		tv[i][1] >>= 1;
		tv[i][2] >>= 1;

		//our implementation requires all input are modulo m
		tv[i][0] = (tv[i][0])%(tv[i][2]);
		tv[i][1] = (tv[i][1])%(tv[i][2]);

		a64[1] = tv[i][0];
		b64[1] = tv[i][1];
		c64[1] = tv[i][2];
		uint64_t res_ref;
		bn_modexp(d,a,b,c);
		bn_modexp_ref64(&res_ref,tv[i][0],tv[i][1],tv[i][2]);
		ASSERT_EQ64(a64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(a64[2],0x892F599F46761CD3);
		ASSERT_EQ64(a64[1],tv[i][0]);
		ASSERT_EQ64(b64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(b64[2],0x892F599F46761CD3);
		ASSERT_EQ64(b64[1],tv[i][1]);
		ASSERT_EQ64(c64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(c64[2],0x892F599F46761CD3);
		ASSERT_EQ64(c64[1],tv[i][2]);
		ASSERT_EQ64(d64[0],0xD6DCB5978DE756ED);ASSERT_EQ64(d64[2],0x892F599F46761CD3);
		ASSERT_EQ64(d64[1],res_ref);
	}
	printf("test_bn_modexp pass\n");
}

static int min_bn_math_self_test(void){
	uint16_t u16 = 0x1234;
	uint8_t *bytes = (uint8_t*)&u16;
	if(bytes[0]!=0x34) {
		printf("Big endian platform: test cannot run on such platform\n");
		return -1;
	}
	printf("\nConstants\n");
	printf("BN_WORD_WIDTH     = ");DBG_PRINT64(BN_WORD_WIDTH);
	printf("BN_BIT            = ");DBG_PRINT64(BN_BIT);
	printf("BN_BYTES          = ");DBG_PRINT64(BN_BYTES);
	printf("BN_WORDS          = ");DBG_PRINT64(BN_WORDS);
	printf("BN_WORDS_MSB_MASK = ");DBG_PRINT32(BN_WORDS_MSB_MASK);
	//All test rely on uint64_t arithmetic being computed right by the test environment
	//Tests are meaningful only with BN_WORD_WIDTH = 8 (to really test the carry propagation and so on)
	ASSERT_EQ64(BN_WORD_WIDTH,8UL);
	ASSERT_EQ64(BN_BIT,64UL);
	printf("\nLaunching unit tests\n");
	test_bn_mov0();
	test_bn_movc();
	test_bn_copy();
	test_bn_shl1();
	test_bn_add();
	test_bn_sub();
	test_bn_cmp();
	test_bn_msb();
	test_bn_mul();
	test_bn_mod();
	test_bn_div();
	test_bn_modmul();
	test_bn_modexp();
	printf("\nmin_bn_math_self_test pass\n");
	return 0;
}
#endif // MIN_BN_MATH_TEST
#endif // __MIN_BN_MATH_H__
