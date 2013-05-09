dnl Check data types

AC_DEFUN([GGI_CHECK_TYPES],
[

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(__int64)
AC_CHECK_SIZEOF(void *, 8)

AC_CHECK_SIZEOF(__darwin_ssize_t)
AC_CHECK_SIZEOF(ssize_t)

if test $ac_cv_sizeof___darwin_ssize_t -ne 0; then
  AC_SUBST([SSIZE_T], [__darwin_ssize_t])
elif test $ac_cv_sizeof_ssize_t -ne 0; then
  AC_SUBST([SSIZE_T], [ssize_t])
else
  AC_CHECK_SIZEOF(SSIZE_T, , [$ac_includes_default
			      #ifdef HAVE_WINDOWS_H
			      #include <windows.h>
			      #endif])
  AC_MSG_CHECKING([for type to use for ssize_t])
  if test $ac_cv_sizeof_SSIZE_T -ne 0; then
    AC_MSG_RESULT([using SSIZE_T])
    AC_SUBST([SSIZE_T], [SSIZE_T])
  else
    AC_MSG_RESULT([none, falling back to type long])
    AC_SUBST([SSIZE_T], [long])
  fi
fi

dnl POSIX: 7.18.1.4 Integer types capable of holding object pointers
AC_CHECK_TYPES([intptr_t, uintptr_t])


if test "$ac_cv_sizeof_int" = "1"; then
  GGI_8="int"
elif test "$ac_cv_sizeof_char" = "1"; then
  GGI_8="char"
elif test "$ac_cv_sizeof_short" = "1"; then
  GGI_8="short"
else
  AC_MSG_ERROR([No 8-bit datatype on this system???])
fi


if test "$ac_cv_sizeof_int" = "2"; then
  GGI_16="int"
elif test "$ac_cv_sizeof_char" = "2"; then
  GGI_16="char"
elif test "$ac_cv_sizeof_short" = "2"; then
  GGI_16="short"
elif test "$ac_cv_sizeof_long" = "2"; then
  GGI_16="long"
else
  AC_MSG_ERROR([No 16-bit datatype on this system???])
fi

if test "$ac_cv_sizeof_int" = "4"; then
  GGI_32="int"
elif test "$ac_cv_sizeof_long" = "4"; then
  GGI_32="long"
elif test "$ac_cv_sizeof_char" = "4"; then
  GGI_32="char"
elif test "$ac_cv_sizeof_short" = "4"; then
  GGI_32="short"
elif test "$ac_cv_sizeof_long_long" = "4"; then
  GGI_32="long long"
else
  AC_MSG_ERROR([No 32-bit datatype on this system???])
fi

if test "$ac_cv_sizeof_int" = "8"; then
  GGI_64="int"
elif test "$ac_cv_sizeof_long" = "8"; then
  GGI_64="long"
elif test "$ac_cv_sizeof_char" = "8"; then
  GGI_64="char"
elif test "$ac_cv_sizeof_short" = "8"; then
  GGI_64="short"
elif test "$ac_cv_sizeof_long_long" = "8"; then
  GGI_64="long long"
elif test "$ac_cv_sizeof___int64" = "8"; then
  GGI_64="__int64"
fi

have_64bit_int="no"
if test -n "$GGI_64"; then
   have_64bit_int="yes"
fi

if test "x$have_64bit_int" = "xno"; then 
  GGI_DEFINE_INT64="#undef"
  AC_MSG_RESULT([No working 64-bit integers.  64-bit code will be disabled.])
else
  GGI_DEFINE_INT64="#define"
  AC_MSG_RESULT([Using type $GGI_64 for 64-bit integers])
fi


dnl POSIX: 7.18.1.4 Integer types capable of holding object pointers

if test "$ac_cv_type_intptr_t" = "yes"; then
  AC_SUBST(INTPTR_T, [intptr_t])
else
  if test "$ac_cv_sizeof_void_p" = "1"; then
    AC_SUBST(INTPTR_T, ["signed $GGI_8"])
  fi
  if test "$ac_cv_sizeof_void_p" = "2"; then
    AC_SUBST(INTPTR_T, ["signed $GGI_16"])
  fi
  if test "$ac_cv_sizeof_void_p" = "4"; then
    AC_SUBST(INTPTR_T, ["signed $GGI_32"])
  fi
  if test "$ac_cv_sizeof_void_p" = "8"; then
    AC_SUBST(INTPTR_T, ["signed $GGI_64"])
  fi
fi

if test "$ac_cv_type_uintptr_t" = "yes"; then
  AC_SUBST(UINTPTR_T, [uintptr_t])
else
  if test "$ac_cv_sizeof_void_p" = "1"; then
    AC_SUBST(UINTPTR_T, ["unsigned $GGI_8"])
  fi
  if test "$ac_cv_sizeof_void_p" = "2"; then
    AC_SUBST(UINTPTR_T, ["unsigned $GGI_16"])
  fi
  if test "$ac_cv_sizeof_void_p" = "4"; then
    AC_SUBST(UINTPTR_T, ["unsigned $GGI_32"])
  fi
  if test "$ac_cv_sizeof_void_p" = "8"; then
    AC_SUBST(UINTPTR_T, ["unsigned $GGI_64"])
  fi
fi

])
