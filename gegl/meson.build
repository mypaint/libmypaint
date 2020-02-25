libmypaint_gegl_inc = include_directories('.')

libmypaint_gegl_sources = [
  '../glib/mypaint-gegl-glib.c',
  'mypaint-gegl-surface.c',
]

libmypaint_gegl_headers = [
  '../glib/mypaint-gegl-glib.h',
  'mypaint-gegl-surface.h',
]

libmypaint_gegl = library('mypaint-gegl-@0@'.format(api_platform_version),
  libmypaint_gegl_sources,
  include_directories: toplevel_inc,
  link_with: libmypaint,
  dependencies: [
    json,
    glib,
    gegl,
  ],
  install: true,
)

install_headers(libmypaint_gegl_headers,
  subdir: 'libmypaint-gegl'
)


if get_option('introspection')
  gnome = import('gnome')

  libmypaint_gegl_gir = gnome.generate_gir(libmypaint_gegl,
    namespace: 'MyPaintGegl',
    nsversion: api_platform_version,

    sources: libmypaint_gegl_sources + libmypaint_gegl_headers,
    symbol_prefix: 'mypaint_gegl',
    identifier_prefix: 'MyPaintGegl',

    includes: [
      'GObject-2.0',
      gegl_gir,
      libmypaint_gir[0],
    ],
    install: true,
  )
endif


pkgconfig.generate(libmypaint_gegl,
  name: meson.project_name() + '-gegl-' + api_platform_version,
  version: version_full,
  description: 'MyPaint brush engine library, with GEGL integration',
  url: project_url,
  subdirs: 'libmypaint-gegl',
)
