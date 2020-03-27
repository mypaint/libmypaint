project('libmypaint',
  'c',
  # API version: see https://github.com/mypaint/libmypaint/wiki/Versioning
  # See http://semver.org/ for what this means.
  version: '2.0.0-beta',
  meson_version: '>=0.49.0',
  default_options: [
    'c_std=c99'
  ],
)

cc = meson.get_compiler('c')

conf = configuration_data()

pkgconfig = import('pkgconfig')

###############################################################################
# Project informations

version_full = meson.project_version()
version_dash_split = version_full.split('-')
version            = version_dash_split[0]
version_prerelease = version_dash_split.get(1, '') # may be blank

version_array = version.split('.')
version_major = version_array[0]
version_minor = version_array[1]
version_micro = version_array[2]

# The API "platform" or "intercompatibility" version.
#
# This one is used for library name prefixes, for introspection
# namespace versions, for gettext domains, and basically anything that
# needs to change when backwards or forwards API compatibility changes.
# Another way of thinking about it: it allows meaningful side by side
# installations of libmypaint.
api_platform_version = '@0@.@1@'.format(version_major, version_minor)
api_name = 'libmypaint-@0@'.format(api_platform_version)

project_url = 'https://github.com/mypaint/libmypaint'

conf.set('PACKAGE_NAME', meson.project_name())
conf.set('PACKAGE_URL', project_url)
conf.set('LIBMYPAINT_API_PLATFORM_VERSION', api_platform_version)
conf.set('LIBMYPAINT_VERSION', version)
conf.set('LIBMYPAINT_VERSION_FULL', version_full)

gettext_package = api_name
conf.set_quoted('GETTEXT_PACKAGE', gettext_package,
  description: 'The prefix for our gettext translation domains.'
)

###############################################################################
# Libtool versionning

# ABI version see: https://autotools.io/libtool/version.html
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
abi_revision = 0  # increment on every release
abi_current = 0   # inc when add/remove/change interfaces
abi_age = 0       # inc only if changes backward compat
# FIXME: Not correct.
abi_version_info = '@0@.@1@.@2@'.format(abi_current, abi_revision, abi_age)

###############################################################################
# System detection, compiler options

platform_win32 = (host_machine.system() == 'windows')
platform_osx = (host_machine.system() == 'darwin')

# Define strdup() in string.h under glibc >= 2.10 (POSIX.1-2008)
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

###############################################################################
# Dependencies

libmath = cc.find_library('m', required: false)

json = dependency('json-c')

# glib
use_glib = get_option('glib') or get_option('introspection')
if use_glib
  glib = dependency('gobject-2.0')
endif
conf.set10('MYPAINT_CONFIG_USE_GLIB', use_glib)

# GEGL
if get_option('gegl')
  gegl = dependency('gegl-0.4', version: '>=0.4', required: false)
  gegl_gir = 'Gegl-0.4'
  if not gegl.found()
    gegl = dependency('gegl-0.3', version: '>=0.3')
    gegl_gir = 'Gegl-0.3'
  endif
else
  gegl = dependency('', required: false)
endif

introspection_required_version = '1.32.0'


# OpenMP
if get_option('openmp')
  # OpenMP requires meson >= 0.46.
  if meson.version().version_compare('<0.46.0')
    error('Meson 0.46.0 is required for OpenMP support.')
  endif
  openmp = dependency('openmp')
else
  openmp = declare_dependency()
endif

## gperftools ##
if get_option('gperftools')
  libprofiler = dependency('libprofiler')
else
  libprofiler = declare_dependency()
endif

# Profiling
if get_option('profiling')
  add_project_arguments('-pg', language: 'c')
endif

# Internationalization
have_i18n = false
if get_option('i18n')
  if not cc.has_header('libintl.h')
    error('libintl.h must be available for i18n support')
  endif
  if platform_win32 or platform_osx
    libintl = cc.find_library('intl')
  else
    libintl = []
  endif
  have_i18n = true
endif
conf.set10('HAVE_GETTEXT', have_i18n)


# Docs
enable_docs = get_option('docs')
if enable_docs
  doxygen = find_program('doxygen')
  sphinx_build = find_program('sphinx-build3', 'sphinx-build-3',
    'sphinx-build2', 'sphinx-build-2', 'sphinx-build'
  )
  # todo: the python 'breathe' extension is also a dependency to doc building.
  # the configure script should check for its existence.
endif


###############################################################################
# Configure files

toplevel_inc = include_directories('.')

configure_file(
  output: 'config.h',
  configuration: conf
)

# TODO change generate.py

configure_file(
  input : 'brushsettings.json',
  output: 'brushsettings.json',
  copy: true,
)
brush_settings_headers = custom_target('brush_settings_headers',
  input: 'brushsettings.json',
  output: [ 'mypaint-brush-settings-gen.h', 'brushsettings-gen.h' ],
  command: [
    find_program('generate.py'), '@OUTPUT@'
  ],
  install: true,
  install_dir: [ get_option('includedir') / api_name, false ],
)


###############################################################################
# Source files

libmypaint_sources = [
  'brushmodes.c',
  'fifo.c',
  'helpers.c',
  'mypaint-brush-settings.c',
  'mypaint-brush.c',
  'mypaint-fixed-tiled-surface.c',
  'mypaint-mapping.c',
  'mypaint-matrix.c',
  'mypaint-rectangle.c',
  'mypaint-surface.c',
  'mypaint-symmetry.c',
  'mypaint-tiled-surface.c',
  'mypaint.c',
  'operationqueue.c',
  'rng-double.c',
  'tilemap.c',
]

libmypaint_introspectable_headers = [
  'mypaint-brush.h',
  'mypaint-brush-settings.h',
  'mypaint-fixed-tiled-surface.h',
  'mypaint-rectangle.h',
  'mypaint-surface.h',
  'mypaint-tiled-surface.h',
]

libmypaint_public_headers = [
  'mypaint-config.h',
  'mypaint-glib-compat.h',
  'mypaint-mapping.h',
  libmypaint_introspectable_headers,
]

install_headers(libmypaint_public_headers,
  subdir: api_name,
)

# Install in subdirectory
if use_glib
  install_headers('glib/mypaint-brush.h',
    subdir: api_name / 'glib',
  )
  libmypaint_introspectable_headers += 'glib/mypaint-brush.h'
  libmypaint_public_headers += 'glib/mypaint-brush.h'
endif

# Do this after because you can't install_headers on a custom_target.
libmypaint_introspectable_headers += brush_settings_headers[0]


libmypaint = library('mypaint-@0@'.format(api_platform_version),
  libmypaint_sources, brush_settings_headers,
  dependencies: [
    glib,
    json,
    libintl,
    libmath,
    openmp,
  ],
  version: abi_version_info,
  install: true,
)


if get_option('introspection')
  gnome = import('gnome')

  libmypaint_gir = gnome.generate_gir(libmypaint,
    nsversion: api_platform_version,
    namespace: 'MyPaint',

    sources: libmypaint_sources + libmypaint_introspectable_headers,
    symbol_prefix: 'mypaint_',
    identifier_prefix: 'MyPaint',

    includes: [
      'GLib-2.0',
      'GObject-2.0',
    ],
    install: true,
  )
endif


pkgconfig.generate(libmypaint,
  name: meson.project_name() + '-' + api_platform_version,
  version: version_full,
  description: 'MyPaint\'s brushstroke rendering library',
  url: project_url,
  subdirs: api_name,
)


if gegl.found()
  subdir('gegl')
endif

if get_option('i18n')
  subdir('po')
endif

subdir('tests')

if enable_docs
  subdir('doc')
endif
