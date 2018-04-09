TESTS = [
  'test-brush-load',
  'test-brush-persistence',
  'test-details',
  'test-fixed-tiled-surface',
  'test-rng',
]

libmypaint_tests_lib = static_library('mypaint-tests',
  'mypaint-benchmark.c',
  'mypaint-benchmark.h',
  'mypaint-test-surface.c',
  'mypaint-test-surface.h',
  'mypaint-utils-stroke-player.c',
  'mypaint-utils-stroke-player.h',
  'testutils.c',
  'testutils.h',
  c_args : '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.source_root()),
  include_directories : toplevel_inc,
  dependencies : GLIB,
)
libmypaint_tests = declare_dependency(
  include_directories : [toplevel_inc, include_directories('.')],
  link_with : libmypaint_tests_lib,
)

foreach name : TESTS
  e = executable(name,
    name + '.c',
    c_args : '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.source_root()),
    include_directories : toplevel_inc,
    link_with : [libmypaint],
    dependencies : [libmypaint_tests, GLIB, MATH, LIBPROFILER],
  )
  test(name, e)
endforeach

subdir('gegl')
