project('libmypaint', 'c',
  # API version: see https://github.com/mypaint/libmypaint/wiki/Versioning
  # See http://semver.org/ for what this means.
  version : '2.0.0-alpha',
  meson_version : '>=0.43.0',
  default_options : ['c_std=c99'])

# API version macros
LIBMYPAINT_VERSION_FULL = meson.project_version()
libmypaint_version_parts = LIBMYPAINT_VERSION_FULL.split('-')
LIBMYPAINT_VERSION = libmypaint_version_parts[0]

libmypaint_api_prerelease = libmypaint_version_parts.get(1, '')  # may be blank
libmypaint_version_parts = LIBMYPAINT_VERSION.split('.')
LIBMYPAINT_MAJOR_VERSION = libmypaint_version_parts[0]
LIBMYPAINT_MINOR_VERSION = libmypaint_version_parts[1]
LIBMYPAINT_MICRO_VERSION = libmypaint_version_parts[2]
# The platform version is "major.minor" only.
# The full version is "major.minor.micro[-prerelease]".

# ABI version see: https://autotools.io/libtool/version.html
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
libmypaint_abi_revision = 0  # increment on every release
libmypaint_abi_current = 0  # inc when add/remove/change interfaces
libmypaint_abi_age = 0  # inc only if changes backward compat
# FIXME: Not correct.
LIBMYPAINT_ABI_VERSION_INFO = '@0@.@1@.@2@'.format(libmypaint_abi_current, libmypaint_abi_revision, libmypaint_abi_age)

# The API "platform" or "intercompatibility" version.
#
# This one is used for library name prefixes, for introspection
# namespace versions, for gettext domains, and basically anything that
# needs to change when backwards or forwards API compatibility changes.
# Another way of thinking about it: it allows meaningful side by side
# installations of libmypaint.
LIBMYPAINT_API_PLATFORM_VERSION = '@0@.@1@'.format(LIBMYPAINT_MAJOR_VERSION, LIBMYPAINT_MINOR_VERSION)

# Dependencies.
gegl_required_version = '0.3'
introspection_required_version = '1.32.0'

conf = configuration_data()
conf.set('PACKAGE_NAME', meson.project_name())
conf.set('PACKAGE_URL', 'https://github.com/mypaint/libmypaint')
conf.set('LIBMYPAINT_API_PLATFORM_VERSION', LIBMYPAINT_API_PLATFORM_VERSION)
conf.set('LIBMYPAINT_VERSION', LIBMYPAINT_VERSION)
conf.set('LIBMYPAINT_VERSION_FULL', LIBMYPAINT_VERSION_FULL)

## Check host and platform ##

cc = meson.get_compiler('c')

PLATFORM_WIN32 = (host_machine.system() == 'windows')
PLATFORM_OSX = (host_machine.system() == 'darwin')

# Define strdup() in string.h under glibc >= 2.10 (POSIX.1-2008)
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')

## Profiling ##
if get_option('profiling')
  add_project_arguments('-pg', language : 'c')
endif

## OpenMP ##
if get_option('openmp')
  # OpenMP requires meson >= 0.46.
  if meson.version().version_compare('<0.46.0')
    error('Meson 0.46.0 is required for OpenMP support.')
  endif
  OPENMP = dependency('openmp')
else
  OPENMP = declare_dependency()
endif

## gperftools ##
if get_option('gperftools')
  LIBPROFILER = dependency('libprofiler')
else
  LIBPROFILER = declare_dependency()
endif

## Docs ##
ENABLE_DOCS = false
if get_option('docs')
  DOXYGEN = find_program('doxygen')
  SPHINX_BUILD = find_program('sphinx-build2', 'sphinx-build-2', 'sphinx-build3', 'sphinx-build-3', 'sphinx-build')
  # TODO: the python 'breathe' extension is also a dependency to doc building.
  # The configure script should check for its existence.
  ENABLE_DOCS = DOXYGEN.found() and SPHINX_BUILD.found()
endif

## json-c ##
JSON = dependency('json-c', required : false)
if not JSON.found()
  message('Could not find \'json-c\', trying legacy \'json\' instead')
  JSON = dependency('json')
endif

## Standard maths functions ##
MATH = cc.find_library('m', required : false)

## Internationalization ##
HAVE_I18N = false
if get_option('i18n')
  GETTEXT_PACKAGE = 'libmypaint-@0@'.format(LIBMYPAINT_API_PLATFORM_VERSION)
  conf.set_quoted('GETTEXT_PACKAGE', GETTEXT_PACKAGE,
    description : 'The prefix for our gettext translation domains.')
  if not cc.has_header('libintl.h')
    error('libintl.h must be available for i18n support')
  endif

  if PLATFORM_WIN32 or PLATFORM_OSX
    LIBINTL = cc.find_library('intl')
  else
    LIBINTL = []
  endif
  HAVE_I18N = true
endif
conf.set10('HAVE_GETTEXT', HAVE_I18N)

## glib ##
if get_option('glib') or get_option('introspection')
  GLIB = dependency('gobject-2.0')
  WITH_GLIB = true
else
  WITH_GLIB = false
endif
conf.set10('MYPAINT_CONFIG_USE_GLIB', WITH_GLIB)

## GEGL ##
ENABLE_GEGL = false
if get_option('gegl')
  GEGL = dependency('gegl-0.3', version : '>=' + gegl_required_version)
  ENABLE_GEGL = true
endif

## configured files ##

config_h = configure_file(
  output : 'config.h',
  configuration : conf)
configure_file(
  input : 'libmypaint.pc.in',
  output : 'libmypaint-@0@.pc'.format(LIBMYPAINT_API_PLATFORM_VERSION),
  configuration : conf,
  install : true,
  install_dir : join_paths(get_option('libdir'), 'pkgconfig'))

## libmypaint ##

toplevel_inc = include_directories('.')

brush_settings_headers = custom_target('brush_settings_headers',
  command : [find_program('generate.py'), '@INPUT@', '@OUTPUT@'],
  input : 'brushsettings.json',
  output : ['mypaint-brush-settings-gen.h', 'brushsettings-gen.h'],
  install : true,
  install_dir : [join_paths(get_option('includedir'), 'libmypaint-@0@'.format(LIBMYPAINT_API_PLATFORM_VERSION)), false])

MyPaint_introspectable_headers = [
  'mypaint-brush.h',
  'mypaint-brush-settings.h',
  'mypaint-fixed-tiled-surface.h',
  'mypaint-rectangle.h',
  'mypaint-surface.h',
  'mypaint-tiled-surface.h',
]

LIBMYPAINT_PUBLIC_HEADERS = [
  'mypaint-config.h',
  'mypaint-glib-compat.h',
  'mypaint-mapping.h',
  MyPaint_introspectable_headers,
]

install_headers(LIBMYPAINT_PUBLIC_HEADERS,
  subdir : 'libmypaint-@0@'.format(LIBMYPAINT_API_PLATFORM_VERSION))
if WITH_GLIB
  install_headers('glib/mypaint-brush.h',
    subdir : 'libmypaint-@0@/glib'.format(LIBMYPAINT_API_PLATFORM_VERSION))
  MyPaint_introspectable_headers += ['glib/mypaint-brush.h']
  LIBMYPAINT_PUBLIC_HEADERS += ['glib/mypaint-brush.h']
endif

# Do this after because you can't install_headers on a custom_target.
MyPaint_introspectable_headers += [brush_settings_headers[0]]
LIBMYPAINT_PUBLIC_HEADERS += [brush_settings_headers[0]]

LIBMYPAINT_SOURCES = [
  'brushmodes.c',
  config_h,
  'fifo.c',
  'helpers.c',
  'mypaint-mapping.c',
  'mypaint.c',
  'mypaint.h',
  'mypaint-brush.c',
  'mypaint-brush-settings.c',
  brush_settings_headers[1],
  'mypaint-fixed-tiled-surface.c',
  'mypaint-rectangle.c',
  'mypaint-surface.c',
  'mypaint-tiled-surface.c',
  'operationqueue.c',
  'rng-double.c',
  'tilemap.c',
  'utils.c',
]

libmypaint = library('mypaint-@0@'.format(LIBMYPAINT_API_PLATFORM_VERSION),
  LIBMYPAINT_SOURCES, LIBMYPAINT_PUBLIC_HEADERS,
  dependencies : [JSON, GLIB, OPENMP, LIBINTL, MATH],
  version : LIBMYPAINT_ABI_VERSION_INFO,
  install : true,
)

## GObject introspection ##

if get_option('introspection')
  gnome = import('gnome')

  libmypaint_gir = gnome.generate_gir(libmypaint,
    sources : MyPaint_introspectable_headers + [
      'brushmodes.c',
      'libmypaint.c',
      'mypaint-brush-settings.c',
      'mypaint-rectangle.c',
      'operationqueue.c',
      'utils.c',
      'fifo.c',
      'mypaint-mapping.c',
      'mypaint.c',
      'mypaint-surface.c',
      'rng-double.c',
      'helpers.c',
      'mypaint-brush.c',
      'mypaint-fixed-tiled-surface.c',
      'mypaint-tiled-surface.c',
      'tilemap.c'
    ],
    includes : ['GObject-2.0', 'GLib-2.0'],
    nsversion : LIBMYPAINT_API_PLATFORM_VERSION,
    namespace : 'MyPaint',
    symbol_prefix : 'mypaint_',
    identifier_prefix : 'MyPaint',
    install : true)
endif

#subdir('doc')
subdir('gegl')
subdir('po')
subdir('tests')
