# AC_OPENMP requires autoconf >= 2.62.
AC_PREREQ(2.62)

m4_define([libmypaint_major_version], [1])
m4_define([libmypaint_minor_version], [3])
m4_define([libmypaint_micro_version], [0])
m4_define([libmypaint_version],
          [libmypaint_major_version.libmypaint_minor_version.libmypaint_micro_version])

# Dependencies.
m4_define([gegl_required_version], [0.3])
m4_define([introspection_required_version], [1.32.0])

AC_INIT([libmypaint], [libmypaint_version],
        [https://github.com/mypaint/libmypaint/issues],
        [libmypaint])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4macros])

AM_INIT_AUTOMAKE([no-define dist-bzip2 dist-xz no-dist-gzip -Wno-portability
                  -Wall -Werror
                  subdir-objects])

LIBMYPAINT_MAJOR_VERSION=libmypaint_major_version
LIBMYPAINT_MINOR_VERSION=libmypaint_minor_version
LIBMYPAINT_MICRO_VERSION=libmypaint_micro_version
LIBMYPAINT_VERSION=libmypaint_version
AC_SUBST(LIBMYPAINT_MAJOR_VERSION)
AC_SUBST(LIBMYPAINT_MINOR_VERSION)
AC_SUBST(LIBMYPAINT_MICRO_VERSION)
AC_SUBST(LIBMYPAINT_VERSION)

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

###########################
# Check target architecture
###########################

AC_MSG_CHECKING([for target architecture])
case x"$target" in
  xNONE | x)
    target_or_host="$host" ;;
  *)
    target_or_host="$target" ;;
esac
AC_MSG_RESULT([$target_or_host])

case "$target_or_host" in
  i*86-*-*)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    ;;
  x86_64-*-*)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    AC_DEFINE(ARCH_X86_64, 1, [Define to 1 if you are compiling for amd64.])
    ;;
  ppc-*-* | powerpc-*)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    ;;
  ppc64-*-* | powerpc64-*)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    AC_DEFINE(ARCH_PPC64, 1, [Define to 1 if you are compiling for PowerPC64.])
    ;;
  *)
    ;;
esac

#################
# Check for Win32
#################

AC_MSG_CHECKING([for some Win32 platform])
case "$target_or_host" in
  *-*-mingw* | *-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for native Win32])
case "$target_or_host" in
  *-*-mingw*)
    os_win32=yes
    case "$host" in
      x86_64-*-*)
	;;
      *)
	WIN32_LARGE_ADDRESS_AWARE='-Wl,--large-address-aware'
	;;
    esac
    PATHSEP=';'
    ;;
  *)
    os_win32=no
    PATHSEP=':'
    ;;
esac
AC_MSG_RESULT([$os_win32])
AC_SUBST(WIN32_LARGE_ADDRESS_AWARE)
AC_SUBST(PATHSEP)
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(OS_UNIX, test "$os_win32" != "yes")

####################
# Check for Mac OS X
####################

platform_osx=no
AC_MSG_CHECKING([if compiling for Mac OS X])
case "$target_or_host" in
  *-*-darwin*)
     AC_MSG_RESULT(yes)
     AC_DEFINE(PLATFORM_OSX, 1, [define to 1 if compiling for Mac OS X])
     platform_osx=yes
     ;;
  *)
     AC_MSG_RESULT(no)
     ;;
esac

AM_CONDITIONAL(PLATFORM_OSX, test "x$platform_osx" = xyes)

# Define strdup() in string.h under glibc >= 2.10 (POSIX.1-2008)
CFLAGS="-D_POSIX_C_SOURCE=200809L $CFLAGS"

## Debug ##
AC_MSG_CHECKING([whether to turn on debugging])
AC_ARG_ENABLE(debug,
              [  --enable-debug          turn on debugging (default=no)])

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
              [  --enable-profiling      turn on profiling (default=no)])

if eval "test x$enable_profiling = xyes"; then
  AC_MSG_RESULT([yes])
  CCFLAGS='-pg $CCFLAGS'
else
  AC_MSG_RESULT([no])
fi

## Variables for pkg-config file ##
PKG_CONFIG_LINKFLAGS="-lm"
PKG_CONFIG_REQUIRES=""

## OpenMP ##
AC_ARG_ENABLE(openmp,
              [  --enable-openmp         compile with OpenMP (default=no)],
              [AC_OPENMP([CFLAGS="$CFLAGS $OPENMP_CFLAGS"])])

AC_SUBST(OPENMP_CFLAGS)

## gperftools ##
AC_ARG_ENABLE(gperftools,
              [  --enable-gperftools     enable gperftools in build, for profiling (default=no)])

have_libprofiler="no"
if test "x$enable_gperftools" = xyes; then
  PKG_CHECK_MODULES(LIBPROFILER, libprofiler,
                    have_libprofiler="yes")
fi
AM_CONDITIONAL(HAVE_GPERFTOOLS, test "x$have_libprofiler" = "xyes")

## Docs ##
AC_ARG_ENABLE(docs,
              [  --enable-docs           enable documentation build (default=no)])

if test "x$enable_docs" = xyes; then
  AC_CHECK_PROGS(DOXYGEN, doxygen)
  AC_CHECK_PROGS(SPHINX_BUILD, sphinx-build2 sphinx-build)
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

## Internationalization ##
AC_ARG_ENABLE(i18n,
              [  --disable-i18n          disable internationalization (default=no)])

if test "x$enable_i18n" != "xno"; then
  enable_i18n="yes"
  GETTEXT_PACKAGE=libmypaint
  AC_SUBST(GETTEXT_PACKAGE)
  AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                     [The prefix for our gettext translation domains.])
  AC_SUBST(GETTEXT_PACKAGE)
  IT_PROG_INTLTOOL
  AM_GLIB_GNU_GETTEXT

  if test "x$platform_osx" = xyes ||
     test "x$platform_win32" = xyes; then
    PKG_CONFIG_LINKFLAGS="$PKG_CONFIG_LINKFLAGS -lintl"
  fi
fi
AM_CONDITIONAL(HAVE_I18N, test "x$enable_i18n" = "xyes")

## Python ##
AM_PATH_PYTHON2

PYTHON="$PYTHON"
AC_SUBST(PYTHON)

GOBJECT_INTROSPECTION_CHECK(introspection_required_version)

## glib ##
AC_ARG_WITH(glib, [  --with-glib             use glib (forced on by introspection)])

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
              [  --enable-gegl           enable GEGL based code in build (yes|no, default=no)])

if eval "test x$enable_gegl = xyes"; then
  PKG_CHECK_MODULES(GEGL, gegl-0.3 >= gegl_required_version)
fi
AM_CONDITIONAL(ENABLE_GEGL, test "x$enable_gegl" = "xyes")

# Set pkg-config variables before generation.
AC_SUBST(PKG_CONFIG_LINKFLAGS)
AC_SUBST(PKG_CONFIG_REQUIRES)

AC_CONFIG_FILES([
  doc/Makefile
  gegl/libmypaint-gegl.pc:gegl/libmypaint-gegl.pc.in
  gegl/Makefile
  libmypaint.pc:libmypaint.pc.in
  m4macros/Makefile
  Makefile
  po/Makefile.in
  tests/Makefile
  tests/gegl/Makefile
])

AC_OUTPUT
