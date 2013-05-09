dnl ---------------------
dnl Check whether usleep returns void, and #define GG_USLEEP_VOID in
dnl that case.  
AC_DEFUN([GGI_USLEEP_VOID],
[AC_CACHE_CHECK([whether usleep returns void], [ggi_cv_usleep_void],
[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT()],[int i; i = usleep(1); return(0);])],
[ggi_cv_usleep_void=no],[ggi_cv_usleep_void=yes])])
dnl usleep on AIX (4.2.1 at least) is broken and sometimes
dnl returns 0 even though the requested time has not elapsed.
dnl Treat usleep on AIX as if it does not return anything at all.
if test "$ggi_cv_usleep_void" = "yes" -o "$os" = "os_aix"; then
  AC_DEFINE(GG_USLEEP_VOID, 1,
            [Define to 1 if the `usleep' function returns void.])
fi
])


dnl ---------------------
dnl Check whether usleep limits value of usecs, and #define GG_USLEEP_999999
dnl in that case.
AC_DEFUN([GGI_USLEEP_999999],
[AC_CACHE_CHECK([whether usleep limits usecs], [ggi_cv_usleep_999999],
[AC_RUN_IFELSE(
[AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT()
#include <errno.h>
],
[int i; i = usleep(1000001); 
 if (i == 0 || i == EINTR) return(-1);
 return 0;])],
[ggi_cv_usleep_999999=yes],
[ggi_cv_usleep_999999=no],
[ggi_cv_usleep_999999=no])])
if test $ggi_cv_usleep_999999 = yes; then
  AC_DEFINE(GG_USLEEP_999999, 1,
            [Define to 1 if the `usleep' function limits usecs value.])
fi
])
