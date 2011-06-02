dnl $Id: configure.ac 3239 2009-10-15 16:35:49Z chris $
AC_INIT([dvdid], [0.2.0a], [cjk32@cam.ac.uk])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/dvdid/main.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_ZIP

AC_C_CONST
AC_C_INLINE

AC_CHECK_HEADER([inttypes.h])

AC_TYPE_UINT8_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T

AC_FUNC_MALLOC


AC_ARG_WITH([windist-binpath],
	[AC_HELP_STRING([--with-windist-binpath@<:@=...@:>@],
		[if building windows distfiles, where to find the compiled binaries @<:@build@:>@])], 
	[WINDIST_BINPATH=$withval], 
	[WINDIST_BINPATH="build"])
AC_SUBST(WINDIST_BINPATH)

dnl Probably shouldn't be abusing AC_ARG_WITH in this way, but it's only for 
dnl internal use, so probably not a problem
AC_ARG_WITH([windist-config],
	[AC_HELP_STRING([--with-windist-config@<:@=...@:>@],
		[which configuration to distribute in dvdid-*.zip @<:@Release@:>@])], 
	[WINDIST_CONFIG=$withval], 
	[WINDIST_CONFIG="Release"]) dnl Release or Debug
AC_SUBST(WINDIST_CONFIG)

AC_ARG_WITH([windist-platform],
	[AC_HELP_STRING([--with-windist-platform@<:@=...@:>@],
		[which platform(s) to distribute as dvdid-*.zip @<:@Win32@:>@])], 
	[WINDIST_PLATFORM=$withval], 
	[WINDIST_PLATFORM="Win32 x64"]) dnl Win32 aor x64
AC_SUBST(WINDIST_PLATFORM)

AC_ARG_WITH([windist-dev-config],
	[AC_HELP_STRING([--with-windist-dev-config@<:@=...@:>@],
		[which configuration(s) to bundle into dvdid-dev-*.zip @<:@Release@:>@])], 
	[WINDIST_DEV_CONFIG=$withval], 
	[WINDIST_DEV_CONFIG="Release"]) dnl Release aor Debug
AC_SUBST(WINDIST_DEV_CONFIG)

AC_ARG_WITH([windist-dev-platform],
	[AC_HELP_STRING([--with-windist-dev-platform@<:@=...@:>@],
		[which platform(s) to bundle into dvdid-dev-*.zip @<:@Win32 x64@:>@])], 
	[WINDIST_DEV_PLATFORM=$withval], 
	[WINDIST_DEV_PLATFORM="Win32 x64"]) dnl Win32 aor x64
AC_SUBST(WINDIST_DEV_PLATFORM)



AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	doc/html/Makefile
	include/Makefile
	include/dvdid/Makefile
	m4/Makefile
	src/Makefile
	src/dvdid/Makefile
	src/libdvdid/Makefile
])
AC_OUTPUT
