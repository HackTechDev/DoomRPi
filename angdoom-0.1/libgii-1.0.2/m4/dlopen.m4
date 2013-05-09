dnl Search for dlopen and friends

AC_DEFUN([GGI_CHECK_DLOPEN],
[

case "${host}" in
#  *-*-mingw* | *-*-cygwin*)
#	;;

  *-*-darwin* | *-*-rhapsody*)
	AC_CHECK_HEADERS(ApplicationServices/ApplicationServices.h	\
		mach-o/dyld.h)
	AC_CHECK_FUNCS(NSCreateObjectFileImageFromFile	\
		NSDestroyObjectFileImage)
	;;

  *)
	AC_CHECK_HEADERS(dlfcn.h link.h)
	AC_CHECK_LIB(c, dlopen, ac_cv_func_dlopen="yes")

	if test "x$ac_cv_func_dlopen" != "xyes"; then
		AC_CHECK_LIB(dl, dlopen,
			     ac_cv_func_dlopen="yes"
			     ac_cv_lib_dl_dlopen="yes")
	fi

	if test "x$ac_cv_lib_dl_dlopen" = "xyes"; then
		GGDLLIBS="$GGDLLIBS -ldl"
	fi

	if test "x$ac_cv_func_dlopen" = "xyes"; then
		AC_DEFINE([HAVE_DLOPEN], [1],
			  [Define if your system has dlopen])
	fi
	;;
esac

])
