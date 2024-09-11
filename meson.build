project(
  'libmypaint',
  'c',
  # API version: see https://github.com/mypaint/libmypaint/wiki/Versioning
  # See http://semver.org/ for what this means.
  version: '2.0.0-beta',
  meson_version: '>=0.60.0',
  default_options: [
    'c_std=c99',
  ],
)

cc = meson.get_compiler('c')

conf = configuration_data()

pkgconfig = import('pkgconfig')
pymod = import('python')

prefix = get_option('prefix')
includedir = prefix / get_option('includedir')
localedir = prefix / get_option('localedir')

###############################################################################
# Project information.

version_full = meson.project_version()
version_dash_split = version_full.split('-')
version_stable = version_dash_split[0]

version_array = version_stable.split('.')
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
api_platform_version = f'@version_major@.@version_minor@'
api_name = f'libmypaint-@api_platform_version@'

project_url = 'https://github.com/mypaint/libmypaint'

conf.set('PACKAGE_NAME', meson.project_name())
conf.set('PACKAGE_URL', project_url)
conf.set('LIBMYPAINT_API_PLATFORM_VERSION', api_platform_version)
conf.set('LIBMYPAINT_VERSION', version_stable)
conf.set('LIBMYPAINT_VERSION_FULL', version_full)

gettext_package = api_name
conf.set_quoted(
  'GETTEXT_PACKAGE',
  gettext_package,
  description: 'The prefix for our gettext translation domains.',
)

###############################################################################
# ABI version. Changes independently of API version.
# See: https://autotools.io/libtool/version.html
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# The rules are fiddly, and are summarized here.

abi_current = 0 # inc when add/remove/change interfaces
abi_revision = 0 # increment on every release
abi_age = 0 # inc only if changes backward compat
abi_soname_version = abi_current - abi_age
abi_version_info = f'@abi_soname_version@.@abi_age@.@abi_revision@'

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
gobject = dependency('gobject-2.0', required: get_option('glib'))
use_glib = gobject.found()
conf.set10('MYPAINT_CONFIG_USE_GLIB', use_glib)

# GEGL
gegl = dependency('gegl-0.4', 'gegl-0.3', required: get_option('gegl'))
use_gegl = gegl.found()
if use_gegl
  gegl_gir = gegl.version().version_compare('>=0.4') ? 'Gegl-0.4' : 'Gegl-0.3'
endif

introspection_required_version = '1.32.0'
introspection_feature = get_option(
  'introspection',
).require(
  use_glib,
  error_message: 'Generating GObject introspection requires building with GLib support',
)
# For g-ir-scanner and g-ir-compiler as used by the gnome module.
gi = dependency(
  'gobject-introspection-1.0',
  version: f'>=@introspection_required_version@',
  required: introspection_feature,
  native: true,
)
use_introspection = gi.found()


# OpenMP
openmp = dependency('openmp', required: get_option('openmp'))

## gperftools ##
libprofiler = dependency('libprofiler', required: get_option('gperftools'))

# Profiling
if get_option('profiling')
  add_project_arguments('-pg', language: 'c')
endif

# Internationalization
cp = find_program('cp', required: false)
msgfmt = find_program('msgfmt', required: false)
msginit = find_program('msginit', required: false)
msgmerge = find_program('msgmerge', required: false)
mv = find_program('mv', required: false)
xgettext = find_program('xgettext', required: false)
fix_po_location = find_program('build-aux/fix-po-location.py')

i18n_feature = get_option(
  'i18n',
).require(
  msgfmt.found(),
  error_message: 'I18n support requires msgfmt from gettext to build mo files',
).require(
  mv.found(),
  error_message: 'I18n support requires mv to install mo files to proper location',
)
libintl = dependency('intl', required: i18n_feature)
use_i18n = libintl.found()
conf.set10('HAVE_GETTEXT', use_i18n)


# Docs
enable_docs = get_option('docs')
if enable_docs
  doxygen = find_program('doxygen')
  sphinx_build = find_program(
    'sphinx-build3',
    'sphinx-build-3',
    'sphinx-build2',
    'sphinx-build-2',
    'sphinx-build',
  )

  python3 = pymod.find_installation(
    'python3',
    modules: [
      'breathe',
    ],
    required: true,
  )

  # todo: the python 'breathe' extension is also a dependency to doc building.
  # the configure script should check for its existence.
endif


###############################################################################
# Configure files

toplevel_inc = include_directories('.')

configure_file(
  output: 'config.h',
  configuration: conf,
)

brush_settings_headers = custom_target(
  'brush_settings_headers',
  input: 'brushsettings.json',
  output: [
    'mypaint-brush-settings-gen.h',
    'brushsettings-gen.h',
  ],
  command: [
    find_program('python3'),
    meson.current_source_dir() / 'generate.py',
    '@OUTPUT@',
  ],
  depend_files: [
    'generate.py',
  ],
  install: true,
  install_dir: [
    includedir / api_name,
    false,
  ],
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
  'mypaint-matrix.h',
  'mypaint-rectangle.h',
  'mypaint-surface.h',
  'mypaint-symmetry.h',
  'mypaint-tiled-surface.h',
]

libmypaint_public_headers = [
  'mypaint-config.h',
  'mypaint-glib-compat.h',
  'mypaint-mapping.h',
  libmypaint_introspectable_headers,
]

install_headers(
  libmypaint_public_headers,
  subdir: api_name,
)

# Install in subdirectory
if use_glib
  install_headers(
    'glib/mypaint-brush.h',
    subdir: api_name / 'glib',
  )
  libmypaint_introspectable_headers += 'glib/mypaint-brush.h'
  libmypaint_public_headers += 'glib/mypaint-brush.h'
endif

# Do this after because you can't install_headers on a custom_target.
libmypaint_introspectable_headers += brush_settings_headers[0]


libmypaint = library(
  f'mypaint-@api_platform_version@',
  libmypaint_sources,
  brush_settings_headers,
  dependencies: [
    gobject,
    json,
    libintl,
    libmath,
    openmp,
  ],
  version: abi_version_info,
  install: true,
)

if use_introspection
  gnome = import('gnome')

  libmypaint_gir = gnome.generate_gir(
    libmypaint,
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


pkgconfig.generate(
  libmypaint,
  name: meson.project_name() + '-' + api_platform_version,
  version: version_full,
  description: 'MyPaint\'s brushstroke rendering library',
  requires: [
    json,
    gobject,
  ],
  url: project_url,
  subdirs: api_name,
)


if use_gegl
  subdir('gegl')
endif

if use_i18n
  subdir('po')
endif

subdir('tests')

if enable_docs
  subdir('doc')
endif
