tests = [
  {
    'name': 'test-brush-load',
    'timeout': 5,
  },
  {
    'name': 'test-brush-persistence',
    'timeout': 5,
  },
  {
    'name': 'test-details',
    'timeout': 60,
  },
  {
    'name': 'test-fixed-tiled-surface',
    'timeout': 1000,
  },
  {
    'name': 'test-rng',
    'timeout': 5,
  },
]

if use_gegl
  tests += {
    'name': 'test-gegl-surface',
    'srcs': 'gegl/test-gegl-surface.c',
    'deps': [
      gegl,
    ],
    'incs': [
      libmypaint_gegl_inc,
    ],
    'link': [
      libmypaint_gegl,
    ],
    'timeout': 2000,
  }
endif

libmypaint_tests_lib = static_library(
  'mypaint-tests',
  'mypaint-benchmark.c',
  'mypaint-test-surface.c',
  'mypaint-utils-stroke-player.c',
  'testutils.c',
  brush_settings_headers,
  c_args: [
    '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.project_source_root()),
  ],
  include_directories: toplevel_inc,
  dependencies: [
    gobject,
  ],
)

foreach test : tests
  test_name = test.get('name')
  test_srcs = test.get('srcs', test_name + '.c')
  test_deps = test.get('deps', [])
  test_incs = test.get('incs', [])
  test_link = test.get('link', [])
  test_timeout = test.get('timeout', 30)

  test_exe = executable(
    test_name,
    test_srcs,
    brush_settings_headers,
    c_args: '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.project_source_root()),
    include_directories: [
      toplevel_inc,
      test_incs,
    ],
    link_with: [
      libmypaint,
      libmypaint_tests_lib,
      test_link,
    ],
    dependencies: [
      gobject,
      libmath,
      libprofiler,
      test_deps,
    ],
  )

  test(
    test_name,
    test_exe,
    timeout: test_timeout,
  )
endforeach
