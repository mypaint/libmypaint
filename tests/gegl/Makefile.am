if ENABLE_GEGL

AM_CPPFLAGS = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/gegl	\
	-I$(top_srcdir)/tests

AM_CFLAGS = -DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR='"@abs_top_srcdir@"'	\
			$(GEGL_CFLAGS) -I$(top_builddir)/gegl/ -I..

TESTS = \
	test-gegl-surface

EXTRA_PROGRAMS = $(TESTS)

CLEANFILES = $(EXTRA_PROGRAMS)

if PLATFORM_WIN32
DEPS = -lintl
else
DEPS = -lm
endif

if HAVE_GPERFTOOLS
DEPS += $(LIBPROFILER_LIBS)
endif

LDADD = \
	$(DEPS)								\
	$(top_builddir)/tests/libmypaint-tests.a \
	$(top_builddir)/libmypaint.la		\
	$(top_builddir)/gegl/libmypaint-gegl.la	\
	$(GEGL_LIBS)

endif
