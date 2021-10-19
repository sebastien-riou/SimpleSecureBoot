/*
 * assert_print.h
 *
 *  Created on: 31 mars 2020
 *      Author: sriou
 */

#ifndef ASSERT_PRINT_H_
#define ASSERT_PRINT_H_

// assume print.h is included before

//#ifndef PP_STRINGIFY
//#define PP_STRINGIFY_IMPL(X) #X
//#define PP_STRINGIFY(X) PP_STRINGIFY_IMPL(X)
//#endif
//
//#ifndef PP_CONCAT
//#define PP_CONCAT_IMPL(x, y) x##y
//#define PP_CONCAT(x, y) PP_CONCAT_IMPL( x, y )
//#endif

#define ADD_PRINT_PREFIX(a) PP_CONCAT(PRINT_PREFIX,a)

//#define _print_enabled       ADD_PRINT_PREFIX(print_enabled      )
//#define _print_cfg           ADD_PRINT_PREFIX(print_cfg          )
//#define _print               ADD_PRINT_PREFIX(print              )
//#define _print_uint_as_hex   ADD_PRINT_PREFIX(print_uint_as_hex  )
//#define _print_uint_as_dec   ADD_PRINT_PREFIX(print_uint_as_dec  )
#define _println             ADD_PRINT_PREFIX(println            )
//#define _print8x             ADD_PRINT_PREFIX(print8x            )
//#define _println8x           ADD_PRINT_PREFIX(println8x          )
//#define _print8d             ADD_PRINT_PREFIX(print8d            )
//#define _println8d           ADD_PRINT_PREFIX(println8d          )
//#define _print16x            ADD_PRINT_PREFIX(print16x           )
//#define _println16x          ADD_PRINT_PREFIX(println16x         )
//#define _print16d            ADD_PRINT_PREFIX(print16d           )
//#define _println16d          ADD_PRINT_PREFIX(println16d         )
//#define _print32x            ADD_PRINT_PREFIX(print32x           )
//#define _println32x          ADD_PRINT_PREFIX(println32x         )
//#define _print32d            ADD_PRINT_PREFIX(print32d           )
#define _println32d          ADD_PRINT_PREFIX(println32d         )
//#define _print64x            ADD_PRINT_PREFIX(print64x           )
//#define _println64x          ADD_PRINT_PREFIX(println64x         )
//#define _print64d            ADD_PRINT_PREFIX(print64d           )
//#define _println64d          ADD_PRINT_PREFIX(println64d         )
//#define _print_bytes_sep     ADD_PRINT_PREFIX(print_bytes_sep    )
//#define _print_bytes         ADD_PRINT_PREFIX(print_bytes        )
#define _println_bytes       ADD_PRINT_PREFIX(println_bytes      )
//#define _print_bytes_0x      ADD_PRINT_PREFIX(print_bytes_0x     )
//#define _println_bytes_0x    ADD_PRINT_PREFIX(println_bytes_0x   )
//#define _print_array32x_0xln ADD_PRINT_PREFIX(print_array32x_0xln)



#ifndef ASSERT_PRINT_FAIL_HOOK
  #define ASSERT_PRINT_FAIL_HOOK(f,l)
#endif

#define ASSERT_EQ( a,b,len) do{assert_eq (a,b,len,__FILE__,__LINE__);}while(0)
#define ASSERT_NEQ(a,b,len) do{assert_neq(a,b,len,__FILE__,__LINE__);}while(0)
#define ASSERT_EQ_VAL(a,b)  do{assert_eq_val (a,b,__FILE__,__LINE__);}while(0)
#define ASSERT_NEQ_VAL(a,b) do{assert_neq_val(a,b,__FILE__,__LINE__);}while(0)
#define ASSERT_TRUE(a)      do{assert_true(a,__FILE__,__LINE__);}while(0)

static void assert_remove_unused_warning();
static void assert_core(const void *a,const void *b, unsigned int len, const char*f,uint32_t l, int eq){
  int status = memcmp(a, b, len);
  int match = status == 0;
  if ( match != eq ){
    ASSERT_PRINT_FAIL_HOOK(f,l);
    _println("");
    _println("ERROR: assert failed");
    if(eq) _println("expected equality");
    else _println("expected inequality");
    _println_bytes("",a,len);
    _println_bytes("",b,len);
    _println(f);
    _println32d("line:",l);
    #ifdef __SBL_H__
    _println("enter_SBL");
    sbl_main();
    #endif
    _println("exit");
    while(1);
    (void)assert_remove_unused_warning;
  }
}

static void assert_neq(const void *a,const void *b, unsigned int len, const char*f,uint32_t l){
  assert_core(a,b,len,f,l,0);
}

static void assert_eq(const void *a,const void *b, unsigned int len, const char*f,uint32_t l){
  assert_core(a,b,len,f,l,1);
}

static void assert_neq_val(uint64_t a, uint64_t b, const char*f,uint32_t l){
  assert_core(&a,&b,sizeof(a),f,l,0);
}

static void assert_eq_val(uint64_t a, uint64_t b, const char*f,uint32_t l){
  assert_core(&a,&b,sizeof(a),f,l,1);
}

static void assert_true(uint64_t a, const char*f,uint32_t l){
  uint64_t false64=0;
  assert_core(&a,&false64,sizeof(false64),f,l,0);
}

static void assert_remove_unused_warning(){
    (void)assert_neq;
    (void)assert_eq;
    (void)assert_neq_val;
    (void)assert_eq_val;
    (void)assert_true;
}


//#undef _print_enabled
//#undef _print_cfg
//#undef _print
//#undef _print_uint_as_hex
//#undef _print_uint_as_dec
#undef _println
//#undef _print8x
//#undef _println8x
//#undef _print8d
//#undef _println8d
//#undef _print16x
//#undef _println16x
//#undef _print16d
//#undef _println16d
//#undef _print32x
//#undef _println32x
//#undef _print32d
#undef _println32d
//#undef _print64x
//#undef _println64x
//#undef _print64d
//#undef _println64d
//#undef _print_bytes_sep
//#undef _print_bytes
#undef _println_bytes
//#undef _print_bytes_0x
//#undef _println_bytes_0x
//#undef _print_array32x_0xln

#undef ADD_PRINT_PREFIX
#undef PRINT_PREFIX

#endif /* ASSERT_PRINT_H_ */
