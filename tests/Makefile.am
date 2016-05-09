AM_CFLAGS = -DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR='"@abs_top_srcdir@"'	\
			$(GLIB_CFLAGS)	\
			-I$(srcdir)/..

TESTS = \
	test-brush-persistence		\
	test-details				\
	test-fixed-tiled-surface	\
	test-rng

EXTRA_PROGRAMS = $(TESTS)

CLEANFILES = $(EXTRA_PROGRAMS)

TESTS_ENVIRONMENT = \
	LIBMYPAINT_TESTING_ABS_TOP_SRCDIR=@abs_top_srcdir@ 

noinst_LIBRARIES = libmypaint-tests.a

libmypaint_tests_a_SOURCES = \
	mypaint-benchmark.c				\
	mypaint-benchmark.h				\
	mypaint-test-surface.c			\
	mypaint-test-surface.h			\
	mypaint-utils-stroke-player.c	\
	mypaint-utils-stroke-player.h	\
	testutils.c						\
	testutils.h

if PLATFORM_WIN32
DEPS = -lintl
else
DEPS = -lm
endif

if HAVE_GPERFTOOLS
DEPS += $(LIBPROFILER_LIBS)
endif

LDADD = \
	$(DEPS)							\
	libmypaint-tests.a				\
	$(top_builddir)/libmypaint.la

EXTRA_DIST = \
	brushes/bulk.myb			\
	brushes/charcoal.myb		\
	brushes/coarse_bulk_2.myb	\
	brushes/impressionism.myb	\
	brushes/modelling.myb		\
	events/painting30sec.dat

SUBDIRS = . gegl
