# AC_OPENMP requires autoconf >= 2.62.
AC_PREREQ(2.62)


## Canonical version number components ##

# API version: see https://github.com/mypaint/libmypaint/wiki/Versioning
# See http://semver.org/ for what this means.

m4_define([libmypaint_api_major], [2])
m4_define([libmypaint_api_minor], [0])
m4_define([libmypaint_api_micro], [0])
m4_define([libmypaint_api_prerelease], [alpha])  # may be blank

# ABI version. Changes independently of API version.
# See: https://autotools.io/libtool/version.html
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# The rules are fiddly, and are summarized here.

m4_define([libmypaint_abi_revision], [0])  # increment on every release
m4_define([libmypaint_abi_current], [0])  # inc when add/remove/change interfaces
m4_define([libmypaint_abi_age], [0])  # inc only if changes backward compat


## Derivative version macros ##

# The full version is "major.minor.micro[-prerelease]".

m4_define([libmypaint_version],
          [libmypaint_api_major.libmypaint_api_minor.libmypaint_api_micro])
m4_define([libmypaint_version_full],
          [libmypaint_api_major().libmypaint_api_minor().libmypaint_api_micro()m4_bpatsubst(libmypaint_api_prerelease(), [^\(.\)], [-\1])])

# The API "platform" or "intercompatibility" version.
#
# This one is used for library name prefixes, for introspection
# namespace versions, for gettext domains, and basically anything that
# needs to change when backwards or forwards API compatibility changes.
# Another way of thinking about it: it allows meaningful side by side
# installations of libmypaint.

m4_define([libmypaint_api_platform_version],
          [libmypaint_api_major.libmypaint_api_minor])


## Dependencies ##

m4_define([gegl_required_version], [0.3])
m4_define([introspection_required_version], [1.32.0])

AC_INIT([libmypaint],
        [libmypaint_version_full],
        [https://github.com/mypaint/libmypaint/issues],
        [libmypaint],
        [https://github.com/mypaint/libmypaint])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4macros])

AM_INIT_AUTOMAKE([no-define dist-xz no-dist-gzip
                  -Wall -Werror
                  subdir-objects
                  foreign])

LIBMYPAINT_MAJOR_VERSION=libmypaint_api_major
LIBMYPAINT_MINOR_VERSION=libmypaint_api_minor
LIBMYPAINT_MICRO_VERSION=libmypaint_api_micro
LIBMYPAINT_VERSION=libmypaint_version
LIBMYPAINT_VERSION_FULL=libmypaint_version_full
LIBMYPAINT_API_PLATFORM_VERSION=libmypaint_api_platform_version
LIBMYPAINT_API_PLATFORM_VERSION_UL=m4_bpatsubst(libmypaint_api_platform_version(), [[^A-Za-z0-9]], [_])
LIBMYPAINT_ABI_VERSION_INFO=libmypaint_abi_current:libmypaint_abi_revision:libmypaint_abi_age

AC_SUBST(LIBMYPAINT_MAJOR_VERSION)
AC_SUBST(LIBMYPAINT_MINOR_VERSION)
AC_SUBST(LIBMYPAINT_MICRO_VERSION)
AC_SUBST(LIBMYPAINT_PRERELEASE_VERSION)
AC_SUBST(LIBMYPAINT_VERSION)
AC_SUBST(LIBMYPAINT_VERSION_FULL)
AC_SUBST(LIBMYPAINT_API_PLATFORM_VERSION)
AC_SUBST(LIBMYPAINT_API_PLATFORM_VERSION_UL)
AC_SUBST(LIBMYPAINT_ABI_VERSION_INFO)

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_AR

AC_PROG_INSTALL

# Initialize libtool (default: shared library only).
LT_PREREQ([2.2])
LT_INIT([disable-static win32-dll])

# Initialize maintainer mode
AM_MAINTAINER_MODE([enable])

# Check for pkg-config
PKG_PROG_PKG_CONFIG(0.16)


## Check host and platform ##

AC_CANONICAL_HOST

AC_MSG_CHECKING([for platform])
platform_win32=no
platform_osx=no
case "$host_os" in
  mingw* | cygwin*)
     AC_MSG_RESULT([win32])
     platform_win32=yes
     ;;
  darwin*)
     AC_MSG_RESULT([osx])
     platform_osx=yes
     ;;
  *)
     AC_MSG_RESULT([unix])
esac
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")
AM_CONDITIONAL(PLATFORM_OSX, test "x$platform_osx" = xyes)

# Define strdup() in string.h under glibc >= 2.10 (POSIX.1-2008)
CFLAGS="-D_POSIX_C_SOURCE=200809L $CFLAGS"

## Debug ##
AC_MSG_CHECKING([whether to turn on debugging])
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug],
                             [turn on debugging (default=no)])
)

if eval "test x$enable_debug = xyes"; then
  AC_MSG_RESULT([yes])
  CPPDEFINES='HEAVY_DEBUG $CPPDEFINES'
  CCFLAGS='-O0 $CCFLAGS'
  LINKFLAGS='-O0 $LINKFLAGS'
else
  AC_MSG_RESULT([no])
  CCFLAGS='-O3 $CCFLAGS'
  LINKFLAGS='-O3 $LINKFLAGS'
fi

## Profiling ##
AC_MSG_CHECKING([whether to turn on profiling])
AC_ARG_ENABLE(profiling,
  AS_HELP_STRING([--enable-profiling],
    [turn on profiling (default=no)])
)

if eval "test x$enable_profiling = xyes"; then
  AC_MSG_RESULT([yes])
  CCFLAGS='-pg $CCFLAGS'
else
  AC_MSG_RESULT([no])
fi

## Variables for pkg-config file ##
PKG_CONFIG_REQUIRES=""

## OpenMP ##
AC_ARG_ENABLE(openmp,
  AS_HELP_STRING([--enable-openmp],
    [compile with OpenMP (default=no)]),
  [AC_OPENMP([CFLAGS="$CFLAGS $OPENMP_CFLAGS"])]
)

AC_SUBST(OPENMP_CFLAGS)

## gperftools ##
AC_ARG_ENABLE(gperftools,
  AS_HELP_STRING([--enable-gperftools],
    [enable gperftools in build, for profiling (default=no)])
)

have_libprofiler="no"
if test "x$enable_gperftools" = xyes; then
  PKG_CHECK_MODULES(LIBPROFILER, libprofiler,
                    have_libprofiler="yes")
fi
AM_CONDITIONAL(HAVE_GPERFTOOLS, test "x$have_libprofiler" = "xyes")

## Docs ##
AC_ARG_ENABLE(docs,
  AS_HELP_STRING([--enable-docs],
    [enable documentation build (default=no)])
)

if test "x$enable_docs" = xyes; then
  AC_CHECK_PROGS(DOXYGEN, doxygen)
  AC_CHECK_PROGS(SPHINX_BUILD, sphinx-build2 sphinx-build-2 sphinx-build3 sphinx-build-3 sphinx-build)
  # TODO: the python "breathe" extension is also a dependency to doc building.
  # The configure script should check for its existence.
fi
AM_CONDITIONAL(ENABLE_DOCS,
               test "x$DOXYGEN" != "x" && test "x$SPHINX_BUILD" != "x")

## json-c ##
PKG_CHECK_MODULES(JSON, json-c,
                  have_json_c="yes", have_json_c="no")

if test "x$have_json_c" = xno; then
  AC_MSG_WARN([Could not find 'json-c', trying legacy 'json' instead])
  PKG_CHECK_MODULES(JSON, json)
  PKG_CONFIG_REQUIRES="json"
else
  PKG_CONFIG_REQUIRES="json-c"
fi

AC_DEFINE(HAVE_JSON_C, 1, [Define to 1 if json is available])
AC_SUBST(JSON_LIBS)
AC_SUBST(JSON_CFLAGS)

## Standard maths functions ##
AC_SEARCH_LIBS([floorf], [m], [], AC_MSG_ERROR([no floorf]))
AC_SEARCH_LIBS([powf], [m], [], AC_MSG_ERROR([no powf]))
AC_SEARCH_LIBS([expf], [m], [], AC_MSG_ERROR([no expf]))
AC_SEARCH_LIBS([fabsf], [m], [], AC_MSG_ERROR([no fabsf]))

## Additional compile flags ##

AX_CHECK_COMPILE_FLAG([-fsanitize=undefined],
  [test x$enable_debug = xyes && CFLAGS="-fsanitize=undefined $CFLAGS"],
  [], [-Werror], [])
AX_CHECK_LINK_FLAG([-fsanitize=undefined],
  [test x$enable_debug = xyes && LDFLAGS="-fsanitize=undefined $LDFLAGS"],
  [], [-Werror], [])

## Internationalization ##
AC_ARG_ENABLE(i18n,
  AS_HELP_STRING([--disable-i18n],
    [disable internationalization (default=no)])
)

if test "x$enable_i18n" != "xno"; then
  enable_i18n="yes"
  GETTEXT_PACKAGE=libmypaint-libmypaint_api_platform_version
  AC_SUBST(GETTEXT_PACKAGE)
  AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                     [The prefix for our gettext translation domains.])
  AC_SUBST(GETTEXT_PACKAGE)
  IT_PROG_INTLTOOL
  AM_GLIB_GNU_GETTEXT

  dnl Debian: stdlib
  dnl Windows, and OSX: -lintl
  AC_SEARCH_LIBS([dgettext], [intl], [], AC_MSG_ERROR([no dgettext]))
fi
AM_CONDITIONAL(HAVE_I18N, test "x$enable_i18n" = "xyes")

GOBJECT_INTROSPECTION_CHECK(introspection_required_version)

## glib ##
AC_ARG_WITH(glib,
  AS_HELP_STRING([--with-glib],
    [use glib (forced on by introspection)])
)

if test "x$with_glib" = xyes ||
   test "x$found_introspection" = xyes; then
  PKG_CHECK_MODULES(GLIB, gobject-2.0)
  AC_DEFINE(MYPAINT_CONFIG_USE_GLIB, 1, [Define to 1 if glib is used])
  PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES gobject-2.0"
else
  AC_DEFINE(MYPAINT_CONFIG_USE_GLIB, 0, [Define to 1 if glib is used])
fi
AM_CONDITIONAL(WITH_GLIB, test "x$with_glib" = "xyes")

## GEGL ##
AC_ARG_ENABLE(gegl,
  AS_HELP_STRING([--enable-gegl],
    [enable GEGL based code in build (yes|no, default=no)])
)

if eval "test x$enable_gegl = xyes"; then
  PKG_CHECK_MODULES(GEGL, gegl-0.3 >= gegl_required_version)
fi
AM_CONDITIONAL(ENABLE_GEGL, test "x$enable_gegl" = "xyes")

# Set pkg-config variables before generation.
AC_SUBST(PKG_CONFIG_REQUIRES)

AC_CONFIG_FILES([
  doc/Makefile
  gegl/libmypaint-gegl-]libmypaint_api_platform_version()[.pc:gegl/libmypaint-gegl.pc.in
  gegl/Makefile
  libmypaint-]libmypaint_api_platform_version()[.pc:libmypaint.pc.in
  m4macros/Makefile
  Makefile
  po/Makefile.in
  tests/Makefile
  tests/gegl/Makefile
])

AC_OUTPUT

echo ""
echo "  Configured libmypaint $LIBMYPAINT_VERSION_FULL"
echo ""

