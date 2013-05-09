
AC_DEFUN([GGI_CC_CAN_INT64],
[AC_CACHE_CHECK([whether $CC thinks type $GGI_64 is a fully working 64-bit int],
        [ggi_cv_cc_can_int64], [
        AC_TRY_LINK([#include <stdio.h>],[
              unsigned $GGI_64 u64; 
              $GGI_64 s64;
              u64 = 0x123456789abcdef0 ; 
              u64 <<= 1; printf("%llx", u64);
	      u64 *= 17; printf("%llx", u64);
              u64 += 300; printf("%llx", u64);
              u64 -= 300; printf("%llx", u64);
              u64 /= 17; printf("%llx", u64);
	      u64 ^= 0xfefefefefefefefe; printf("%llx", u64);
	      u64 %= 3; printf("%llx", u64);
              s64 = -0x123456789abcdef0; 
              s64 <<= 1; printf("%llx", s64);
	      s64 *= 17; printf("%llx", s64);
              s64 += 300; printf("%llx", s64);
              s64 -= 300; printf("%llx", s64);
              s64 /= 17; printf("%llx", s64);
	      s64 ^= 0xfefefefefefefefe; printf("%llx", s64);
	      s64 %= 3; printf("%llx", s64);
	      s64 *= u64; printf("%llx", s64);
              return u64;],
           ggi_cv_cc_can_int64="yes",
           ggi_cv_cc_can_int64="no")])
])

AC_DEFUN([GGI_CC_CAN_CPUID],
[AC_CACHE_CHECK([whether $CC can assemble cpuid instruction],
	[ggi_cv_cc_can_cpuid],
	[ AC_TRY_LINK(,[unsigned long a,b,c,d,in; 
	#ifdef _MSC_VER
	__asm cpuid
	#else
	asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));
	#endif
	return d;],
	ggi_cv_cc_can_cpuid="yes", ggi_cv_cc_can_cpuid="no")])
	if test "x$ggi_cv_cc_can_cpuid" = "xyes"; then
	     AC_DEFINE(CC_CAN_CPUID, 1,
		       [Define if CC can compile cpuid opcode (Intel)])
	fi
])

AC_DEFUN([GGI_CC_CAN_AMASK],
[AC_CACHE_CHECK([whether $CC can assemble amask instruction],
	[ggi_cv_cc_can_amask],
	[ AC_TRY_LINK([#ifdef __DECC
	#include <c_asm.h>
	#endif
	],[unsigned long a,b; 
	#ifdef __DECC
	asm("amask %1, %0", (a), (b));
	#else
	asm("amask %1, %0": "=r" (a) : "ri" (b));
	#endif
	return a;],
	ggi_cv_cc_can_amask="yes", ggi_cv_cc_can_amask="no")])
	if test "x$ggi_cv_cc_can_amask" = "xyes"; then
	     AC_DEFINE(CC_CAN_AMASK, 1,
		       [Define if CC can compile amask opcode (DEC)])
	fi
])
