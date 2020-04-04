# Process this file with autoconf to create configure.

AC_PREREQ([2.65])

# ====================
# Version informations
# ====================
m4_define([libstaroffice_version_major],[0])
m4_define([libstaroffice_version_minor],[0])
m4_define([libstaroffice_version_micro],[7])
m4_define([libstaroffice_version],[libstaroffice_version_major.libstaroffice_version_minor.libstaroffice_version_micro])

# =============
# Automake init
# =============
AC_INIT([libstaroffice],[libstaroffice_version])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([1.11 foreign dist-xz dist-bzip2])
AM_SILENT_RULES([yes])
AC_LANG([C++])

# ===========================
# Find required base packages
# ===========================
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT([win32-dll disable-static pic-only])
AC_CANONICAL_HOST

AX_CXX_COMPILE_STDCXX_11
AX_GCC_FUNC_ATTRIBUTE([format])
DLP_FALLTHROUGH

PKG_PROG_PKG_CONFIG([0.20])

# =================================
# Libtool/Version Makefile settings
# =================================
AC_SUBST(STAROFFICE_MAJOR_VERSION, [libstaroffice_version_major])
AC_SUBST(STAROFFICE_MINOR_VERSION, [libstaroffice_version_minor])
AC_SUBST(STAROFFICE_MICRO_VERSION, [libstaroffice_version_micro])
AC_SUBST(STAROFFICE_VERSION, [libstaroffice_version])
# AC_SUBST(LT_RELEASE, [libstaroffice_version_major.libstaroffice_version_minor])
LT_CURRENT=`expr 100 '*' libstaroffice_version_major + libstaroffice_version_minor`
# For 1.0.0 comment the first line and uncomment the second
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION, [libstaroffice_version_micro])
AC_SUBST(LT_AGE)

# ==========================
# Platform check for windows
# ==========================
AC_MSG_CHECKING([for native Win32])
AS_CASE([$host],
	[*-*-mingw*], [
		native_win32=yes
		LIBSTAROFFICE_WIN32_RESOURCE=libstaroffice-win32res.lo
		LIBSTAROFFICE_STREAM_WIN32_RESOURCE=libstaroffice-stream-win32res.lo
		SDC2CSV_WIN32_RESOURCE=sdc2csv-win32res.lo
		SDW2HTML_WIN32_RESOURCE=sdw2html-win32res.lo
		SD2RAW_WIN32_RESOURCE=sd2raw-win32res.lo
		SD2SVG_WIN32_RESOURCE=sd2svg-win32res.lo
		SD2TEXT_WIN32_RESOURCE=sd2text-win32res.lo
	], [
		native_win32=no
		LIBSTAROFFICE_WIN32_RESOURCE=
		LIBSTAROFFICE_STREAM_WIN32_RESOURCE=
		SDC2CSV_WIN32_RESOURCE=
		SDW2HTML_WIN32_RESOURCE=
		SD2RAW_WIN32_RESOURCE=
		SD2SVG_WIN32_RESOURCE=
		SD2TEXT_WIN32_RESOURCE=
	]
)
AM_CONDITIONAL(OS_WIN32, [test "x$native_win32" = "xyes"])
AC_SUBST(LIBSTAROFFICE_WIN32_RESOURCE)
AC_SUBST(LIBSTAROFFICE_STREAM_WIN32_RESOURCE)
AC_SUBST(SDC2CSV_WIN32_RESOURCE)
AC_SUBST(SDW2HTML_WIN32_RESOURCE)
AC_SUBST(SD2RAW_WIN32_RESOURCE)
AC_SUBST(SD2SVG_WIN32_RESOURCE)
AC_SUBST(SD2TEXT_WIN32_RESOURCE)

AC_MSG_CHECKING([for Win32 platform in general])
AS_CASE([$host],
	[*-*-mingw*|*-*-cygwin*], [platform_win32=yes],
	[platform_win32=no]
)
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" = "xyes"])

AS_IF([test "x$platform_win32" = "xyes"],
    [],
    [
        AC_MSG_CHECKING([for -fvisibility=hidden compiler flag])
        saved_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
        AC_TRY_COMPILE([], [],
            [
                AC_MSG_RESULT([yes])
                CXXFLAGS="$CXXFLAGS -DLIBSTAROFFICE_VISIBILITY"
            ],
            [
                AC_MSG_RESULT([no])
                CXXFLAGS="$saved_CXXFLAGS"
            ]
        )
    ]
)

# =========================
# Platform check for darwin
# =========================
AC_MSG_CHECKING([for Darwin (Mac OS X) platform])
AS_CASE([$host],
	[*-*-darwin*], [platform_darwin=yes],
	[platform_darwin=no]
)
AC_MSG_RESULT([$platform_darwin])
AM_CONDITIONAL(PLATFORM_DARWIN, [test "x$platform_darwin" = "xyes"])

# ================
# Check for cflags
# ================
AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--enable-werror], [Treat all warnings as errors, usefull for development])],
	[enable_werror="$enableval"],
	[enable_werror=no]
)
AS_IF([test x"$enable_werror" != "xno"], [
	CFLAGS="$CFLAGS -Werror"
	CXXFLAGS="$CXXFLAGS -Werror"
])
AC_ARG_ENABLE([weffc],
	[AS_HELP_STRING([--disable-weffc], [ Disable -Weffc++ warnings, usefull when using an old version of gcc or of boost])],
	[enable_weffc="$enableval"],
	[enable_weffc=yes]
)
AC_ARG_ENABLE([wparanoic],
	[AS_HELP_STRING([--enable-wparanoic], [Enable a lot of warnings...])],
	[enable_wparanoic="$enableval"],
	[enable_wparanoic=no]
)
# Courtesy of Glib: Ensure MSVC-compatible struct packing convention
# is used when compiling for Win32 with gcc.
AS_IF([test "x$native_win32" = "xyes"], [
	AC_CHECK_TOOL(WINDRES, windres)
	AS_IF([test x"$GCC" = xyes], [
		AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
		AS_IF([test -z "$ac_cv_prog_CC"], [
			our_gcc="$CC"
		], [
			our_gcc="$ac_cv_prog_CC"
		])
		AS_IF([$our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null], [
			msnative_struct="-mms-bitfields"
			CFLAGS="$CFLAGS $msnative_struct"
			CXXFLAGS="$CXXFLAGS $msnative_struct"
			AC_MSG_RESULT([${msnative_struct}])
		], [
			AC_MSG_RESULT([no way])
			AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
		])
	])
	CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
	CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic"
], [
	AS_IF([test x"$GCC" = xyes], [
		# clang or gcc, check if the compiler is clang
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM(
				[[]],
				[[#ifndef __clang__]
				 [   not clang]
				 [#endif]]
			)],
		[CLANG=yes], [CLANG=no])
		# Be tough with warnings and produce less careless code
		CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
		CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Wshadow -Wunused-variable"
		AS_IF([test x"$enable_weffc" != "xno"], [
			CXXFLAGS="$CXXFLAGS -Weffc++"
		])
		AS_IF([test x"$enable_wparanoic" != "xno"], [
		# uncomment the 5 following comments to compile with Weverything
		#AS_IF([test x"$CLANG" = "xyes"], [
		#	CXXFLAGS="$CXXFLAGS -Weverything -Wno-padded -Wno-covered-switch-default --system-header-prefix=librevenge"
		#	],
		#	[
			CXXFLAGS="$CXXFLAGS -Wcast-align -Wcast-qual -Wchar-subscripts"
			CXXFLAGS="$CXXFLAGS -Wcomment -Wconversion -Wdisabled-optimization"
			CXXFLAGS="$CXXFLAGS -Wfloat-equal -Wformat -Wformat=2"
			CXXFLAGS="$CXXFLAGS -Wformat-nonliteral -Wformat-security"
			CXXFLAGS="$CXXFLAGS -Wformat-y2k -Wimport -Winit-self -Winvalid-pch"
			CXXFLAGS="$CXXFLAGS -Wmissing-braces -Wmissing-field-initializers"
			CXXFLAGS="$CXXFLAGS -Wmissing-format-attribute -Wmissing-include-dirs"
			CXXFLAGS="$CXXFLAGS -Wmissing-noreturn -Wpacked -Wparentheses"
			CXXFLAGS="$CXXFLAGS -Wpointer-arith -Wredundant-decls -Wreturn-type"
			CXXFLAGS="$CXXFLAGS -Wsequence-point -Wsign-compare -Wstrict-aliasing"
			CXXFLAGS="$CXXFLAGS -Wstrict-aliasing=2 -Wswitch -Wswitch-default"
			CXXFLAGS="$CXXFLAGS -Wswitch-enum -Wtrigraphs -Wunknown-pragmas -Wunused"
			CXXFLAGS="$CXXFLAGS -Wunused-function -Wunused-label -Wunused-parameter"
			CXXFLAGS="$CXXFLAGS -Wunused-value -Wvariadic-macros"
			CXXFLAGS="$CXXFLAGS -Wvolatile-register-var -Wwrite-strings"
		#       ])
		])
	])
])

# =======
# Zlib
# =======
AC_ARG_ENABLE([zip],
	[AS_HELP_STRING([--disable-zip], [Do not use the zip library.])],
	[with_zip="$enableval"],
	[with_zip=yes]
)
if test "x$with_zip" != "xno"; then
	PKG_CHECK_MODULES([ZLIB],[zlib])
	AC_DEFINE([USE_ZIP],[1],[Use the zip library to create mwawZip.])
else
	ZLIB_CFLAGS=
	ZLIB_LIBS=
fi
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)

# ============
# Debug switch
# ============
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug], [Turn on debugging])],
	[enable_debug="$enableval"],
	[enable_debug=no]
)
AC_ARG_ENABLE([full-debug],
	[AS_HELP_STRING([--enable-full-debug], [Turn on debugging and debug files' creation ])],
	[enable_full_debug="$enableval"],
	[enable_full_debug=no]
)
AS_IF([test "x$enable_full_debug" = "xyes"],
	[ DEBUG_CXXFLAGS="-DDEBUG -DDEBUG_WITH_FILES -g" ],
	[ AS_IF([test "x$enable_debug" = "xyes"],
		[DEBUG_CXXFLAGS="-DDEBUG -g"],
		[DEBUG_CXXFLAGS="-DNDEBUG"])
	]
)
AC_SUBST(DEBUG_CXXFLAGS)

# ============
# Static tools
# ============
AC_ARG_ENABLE([static-tools],
	[AS_HELP_STRING([--enable-static-tools], [Link tools (binaries) statically])],
	[enable_static_tools="$enableval"],
	[enable_static_tools=no]
)
AS_IF([test "x$enable_static_tools" = "xyes"], [
	enable_static="yes"
], [
	AC_DISABLE_STATIC
])
AM_CONDITIONAL(STATIC_TOOLS, [test "x$enable_static_tools" = "xyes"])

# =============
# Documentation
# =============
AC_ARG_WITH(docs,
	[AS_HELP_STRING([--without-docs], [Do not build documentation])],
	[with_docs="$withval"],
	[AS_IF([test "x$native_win32" = "xyes"], [with_docs=no], [with_docs=yes])]
)
AS_IF([test "x$with_docs" != "xno"], [
	AC_PATH_PROG(DOXYGEN, [doxygen])
	AS_IF([test -z "$DOXYGEN"], [
		AC_MSG_WARN([*** Could not find doxygen in your PATH.])
		AC_MSG_WARN([*** The documentation will not be built.])
		build_docs=no
	], [build_docs=yes])
], [build_docs=no])
AM_CONDITIONAL([WITH_LIBSTAROFFICE_DOCS], [test "x$build_docs" != "xno"])


# ====================
# Find librevenge
# ====================
PKG_CHECK_MODULES([REVENGE],[ librevenge-0.0 ])
AC_SUBST(REVENGE_CFLAGS)
AC_SUBST(REVENGE_LIBS)

# =====
# Tools
# =====
AC_ARG_ENABLE([tools],
	[AS_HELP_STRING([--disable-tools], [Build conversion tools])],
	[enable_tools="$enableval"],
	[enable_tools=yes]
)
AM_CONDITIONAL(BUILD_TOOLS, [test "x$enable_tools" = "xyes"])

# =======
# Fuzzers
# =======
AC_ARG_ENABLE([fuzzers],
	[AS_HELP_STRING([--enable-fuzzers], [Build fuzzer(s)])],
	[enable_fuzzers="$enableval"],
	[enable_fuzzers=no]
)
AM_CONDITIONAL(BUILD_FUZZERS, [test "x$enable_fuzzers" = "xyes"])

AS_IF([test "x$enable_tools" = "xyes" -o "x$enable_fuzzers" = "xyes"], [
	PKG_CHECK_MODULES([REVENGE_GENERATORS],[
		librevenge-generators-0.0
	])
	PKG_CHECK_MODULES([REVENGE_STREAM],[
		librevenge-stream-0.0
	])
])
AC_SUBST([REVENGE_GENERATORS_CFLAGS])
AC_SUBST([REVENGE_GENERATORS_LIBS])
AC_SUBST([REVENGE_STREAM_CFLAGS])
AC_SUBST([REVENGE_STREAM_LIBS])

# =====================
# Prepare all .in files
# =====================
AC_CONFIG_FILES([
Makefile
inc/Makefile
inc/libstaroffice/Makefile
src/Makefile
src/conv/Makefile
src/conv/sdc2csv/Makefile
src/conv/sdc2csv/sdc2csv.rc
src/conv/sdw2html/Makefile
src/conv/sdw2html/sdw2html.rc
src/conv/sd2raw/Makefile
src/conv/sd2raw/sd2raw.rc
src/conv/sd2svg/Makefile
src/conv/sd2svg/sd2svg.rc
src/conv/sd2text/Makefile
src/conv/sd2text/sd2text.rc
src/fuzz/Makefile
src/lib/Makefile
src/lib/libstaroffice.rc
docs/Makefile
docs/doxygen/Makefile
build/Makefile
build/win32/Makefile
libstaroffice-$STAROFFICE_MAJOR_VERSION.$STAROFFICE_MINOR_VERSION.pc:libstaroffice.pc.in
libstaroffice-zip
])
AC_OUTPUT

# ==============================================
# Display final informations about configuration
# ==============================================
AC_MSG_NOTICE([
==============================================================================
Build configuration:
	debug:           ${enable_debug}
	full-debug:      ${enable_full_debug}
	docs:            ${build_docs}
	fuzzers:         ${enable_fuzzers}
	zip:             ${with_zip}
	static-tools:    ${enable_static_tools}
	werror:          ${enable_werror}
==============================================================================
])
