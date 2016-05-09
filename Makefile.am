libmypaint_glib = glib/mypaint-brush.h

## GObject introspection ##

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --pkg="glib-2.0" \
							 --namespace="MyPaint" --nsversion="$(LIBMYPAINT_VERSION)" \
							 --identifier-prefix="MyPaint" --symbol-prefix="mypaint_" \
							 --add-include-path="$(srcdir)" \
							 --add-include-path="`$(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0`"
INTROSPECTION_COMPILER_ARGS = --includedir="$(srcdir)" \
							  --includedir="`$(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0`"
INTROSPECTION_SCANNER_ENV = CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS) -lm" CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)"

MyPaint_introspectable_headers = \
	mypaint-brush.h					\
	mypaint-brush-settings.h		\
	mypaint-brush-settings-gen.h	\
	mypaint-fixed-tiled-surface.h	\
	mypaint-rectangle.h				\
	mypaint-surface.h				\
	mypaint-tiled-surface.h			\
	$(libmypaint_glib)

if HAVE_INTROSPECTION

introspection_sources = \
	$(MyPaint_introspectable_headers)	\
	brushmodes.c					\
	libmypaint.c					\
	mypaint-brush-settings.c		\
	mypaint-rectangle.c				\
	operationqueue.c				\
	utils.c							\
	fifo.c							\
	mapping.c						\
	mypaint.c						\
	mypaint-surface.c				\
	rng-double.c					\
	helpers.c						\
	mypaint-brush.c					\
	mypaint-fixed-tiled-surface.c	\
	mypaint-tiled-surface.c			\
	tilemap.c

MyPaint-@LIBMYPAINT_VERSION@.gir: libmypaint.la Makefile
MyPaint_@LIBMYPAINT_MAJOR_VERSION@_@LIBMYPAINT_MINOR_VERSION@_gir_INCLUDES = GObject-2.0 GLib-2.0
MyPaint_@LIBMYPAINT_MAJOR_VERSION@_@LIBMYPAINT_MINOR_VERSION@_gir_CFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
MyPaint_@LIBMYPAINT_MAJOR_VERSION@_@LIBMYPAINT_MINOR_VERSION@_gir_LIBS = libmypaint.la
MyPaint_@LIBMYPAINT_MAJOR_VERSION@_@LIBMYPAINT_MINOR_VERSION@_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += MyPaint-@LIBMYPAINT_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(gir_DATA) $(typelib_DATA)

endif # HAVE_INTROSPECTION

## pkg-config file ##

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libmypaint.pc

## Python files ##

pydir = $(datadir)/libmypaint

py_DATA = \
	__init__.py			\
	brushsettings.json	\
	brushsettings.py

## libmypaint ##

AM_CFLAGS = $(JSON_CFLAGS) $(GLIB_CFLAGS)

LIBS = $(JSON_LIBS) $(GLIB_LIBS)

lib_LTLIBRARIES = libmypaint.la

libmypaint_publicdir = $(includedir)/libmypaint

nobase_libmypaint_public_HEADERS = \
	mypaint-config.h				\
	mypaint-glib-compat.h			\
	$(MyPaint_introspectable_headers)

LIBMYPAINT_SOURCES = \
	brushmodes.c					\
	fifo.c							\
	helpers.c						\
	mapping.c						\
	mypaint.c						\
	mypaint.h						\
	mypaint-brush.c					\
	mypaint-brush-settings.c		\
	mypaint-fixed-tiled-surface.c	\
	mypaint-rectangle.c				\
	mypaint-surface.c				\
	mypaint-tiled-surface.c			\
	operationqueue.c				\
	rng-double.c					\
	tilemap.c						\
	utils.c

libmypaint_la_SOURCES = $(libmypaint_public_HEADERS) $(LIBMYPAINT_SOURCES)

brushsettings-gen.h mypaint-brush-settings-gen.h:
	$(PYTHON) generate.py mypaint-brush-settings-gen.h brushsettings-gen.h

mypaint-config.h: mypaint-config.h.top config.h mypaint-config.h.bottom
	cat $^ >$@

EXTRA_DIST = \
	COPYING							\
	README.md						\
	brushmodes.h					\
	fifo.h							\
	helpers.h						\
	libmypaint.c					\
	mapping.h						\
	operationqueue.h				\
	rng-double.h					\
	tiled-surface-private.h			\
	tilemap.h						\
	utils.h							\
	glib/mypaint-brush.c

if HAVE_I18N
PODIRS = po
endif

SUBDIRS = \
	.		\
	doc		\
	gegl	\
	tests	\
	$(PODIRS)