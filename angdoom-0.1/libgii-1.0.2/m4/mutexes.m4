dnl Optional: Predefine $mutextype
dnl Return: $mutextype and $THREADLIBS

AC_DEFUN([GGI_MUTEXTYPE],
[

dnl Valid mutextype list
valid_mutextypes="yes pthread win32 builtin"

dnl Set mutextype to autodetect (yes) if not already
dnl specified via configure
if test -z "$mutextype" -o "$mutextype" = "auto"; then
	mutextype="yes"
fi

dnl Check, if the user specified a valid value
found=0
for mtype in $valid_mutextypes; do
	if test "$mtype" = "$mutextype"; then
		found=1
	fi
done
if test $found -eq 0; then
	AC_MSG_ERROR([Unsupported mutex type passed to --enable-mutexes: $mutextype
			Supported mutex types: $valid_mutextypes])
fi

THREADLIBS=""


dnl Check for pthread library

if test "$mutextype" = "yes" -o \
	"$mutextype" = "pthread"; then

AC_CHECK_HEADERS([pthread.h])
have_pthread=no
if test "$ac_cv_header_pthread_h" = "yes"; then
	TMP_SAVE_LIBS=$LIBS
	TMP_SAVE_CC=$CC
	CC="$SHELL ./libtool --mode=link $CC"

	for pthreadlib in -lpthread -pthread -lc_r; do
		AC_MSG_CHECKING(for pthread_mutex_init with $pthreadlib)
		LIBS="$TMP_SAVE_LIBS $pthreadlib"

		AC_TRY_RUN([
			#define __C_ASM_H /* fix for retarded Digital Unix headers */
			#include <pthread.h>
			pthread_mutex_t mtex;
			int main(void)
			{
				if (pthread_mutex_init(&mtex, NULL) == 0) return 0;
				return 1;
			}
		],[
			AC_MSG_RESULT(yes)
			have_pthread=yes
			THREADLIBS="$pthreadlib"
			break
		],[
			AC_MSG_RESULT(no)
		])
	done

	LIBS=$TMP_SAVE_LIBS
	CC="$TMP_SAVE_CC"
fi

dnl Use pthread library if present
if test "$have_pthread" = "yes"; then
	mutextype="pthread"
elif test "$mutextype" = "pthread"; then
	mutextype="no"
fi

fi


dnl Check for OS specific thread support

if test "$mutextype" = "yes" -o \
	"$mutextype" = "win32"; then

dnl Win32 (after pthread since pthread is prefered on cygwin)
AC_CHECK_HEADERS([windows.h])
AC_MSG_CHECKING(for win32 semaphores)
if test "x$ac_cv_header_windows_h" = "xyes"; then
	AC_MSG_RESULT(yes)
	mutextype="win32"
	THREADLIBS=""
else
	AC_MSG_RESULT(no)
	if test "$mutextype" = "win32"; then
		mutextype="no"
	fi
fi

fi


dnl Fall back to builtin mutexes, if nothing else was found

if test "$mutextype" = "yes" -o \
	"$mutextype" = "no"; then
	mutextype="builtin"
fi

AC_SUBST(THREADLIBS)

])
