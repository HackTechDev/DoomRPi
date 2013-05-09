dnl AC_CHECK_WINFUNC(FUNCTION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN([AC_CHECK_WINFUNC],
[
ldflags_old=$LDFLAGS
LDFLAGS="$LDFLAGS -lkernel32"

AC_MSG_CHECKING([for $1 (winfunc)])
if eval "test \"`echo 'x$ac_cv_func_'$1`\" = xno"; then
  eval "unset ac_cv_func_$1"
fi
AC_CACHE_VAL(ac_cv_func_$1,
[AC_TRY_LINK([
#include <assert.h>
#include <windows.h>
]ifelse(test, test, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[
], [
void *foopointer = NULL;
$1(foopointer);
], eval "ac_cv_func_$1=yes", AC_TRY_LINK([
#include <assert.h>
#include <windows.h>
]ifelse(test, test, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[
], [
$1();
], eval "ac_cv_func_$1=yes", AC_TRY_LINK([
#include <assert.h>
#include <windows.h>
]ifelse(test, test, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[
], [
long fooint = 0;
$1(fooint);
], eval "ac_cv_func_$1=yes", AC_TRY_LINK([
#include <assert.h>
#include <windows.h>
]ifelse(test, test, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[
], [
void *foopointer = NULL;
$1(foopointer,foopointer);
], eval "ac_cv_func_$1=yes", eval "ac_cv_func_$1=no"))))])
if eval "test \"`echo '$ac_cv_func_'$1`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi

LDFLAGS=$ldflags_old
])

dnl AC_CHECK_WINFUNCS(FUNCTION... [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN([AC_CHECK_WINFUNCS],
[AC_CHECK_FUNCS([$1],[$2],[$3])
if test "x$ac_cv_header_windows_h" = "xyes"; then
for ac_func in $1
do
AC_CHECK_WINFUNC($ac_func,
[changequote(, )dnl
  ac_tr_func=HAVE_`echo $ac_func | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_func) $2], $3)dnl
done
fi
])
