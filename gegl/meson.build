# Optional
if ENABLE_GEGL

## pkg-config file ##
configure_file(
  input : 'libmypaint-gegl.pc.in',
  output : 'libmypaint-gegl-@0@.pc'.format(LIBMYPAINT_API_PLATFORM_VERSION),
  configuration : conf,
  install : true,
  install_dir : join_paths(get_option('libdir'), 'pkgconfig'))

## libmypaint-gegl ##

libmypaint_gegl_inc = include_directories('.')

libmypaint_gegl_introspectable_headers = [
  '../glib/mypaint-gegl-glib.h',
  'mypaint-gegl-surface.h',
]

libmypaint_gegl_public_HEADERS = libmypaint_gegl_introspectable_headers
install_headers(libmypaint_gegl_public_HEADERS, subdir : 'libmypaint-gegl')

LIBMYPAINT_GEGL_SOURCES = [
  '../glib/mypaint-gegl-glib.c',
  'mypaint-gegl-surface.c',
]

libmypaint_gegl = library('mypaint-gegl-@0@'.format(LIBMYPAINT_API_PLATFORM_VERSION),
  LIBMYPAINT_GEGL_SOURCES, libmypaint_gegl_public_HEADERS,
  include_directories : toplevel_inc,
  dependencies : [JSON, GLIB, GEGL],
  link_with : libmypaint,
  install : true,
)

## GObject introspection ##

if get_option('introspection')
  gnome = import('gnome')

  libmypaint_gegl_gir = gnome.generate_gir(libmypaint_gegl,
    sources : libmypaint_gegl_introspectable_headers + [
      '../glib/mypaint-gegl-glib.c',
      'mypaint-gegl-surface.c',
    ],
    includes : ['GObject-2.0', libmypaint_gir[0], 'Gegl-0.3'],
    nsversion : '@0@.@1@'.format(LIBMYPAINT_MAJOR_VERSION, LIBMYPAINT_MINOR_VERSION),
    namespace : 'MyPaintGegl',
    symbol_prefix : 'mypaint_gegl',
    identifier_prefix : 'MyPaintGegl',
    install : true)
endif

endif # enable_gegl
