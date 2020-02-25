tests = [
    { 'name': 'test-brush-load',
  },{ 'name': 'test-brush-persistence',
  },{ 'name': 'test-details',
  },{ 'name': 'test-fixed-tiled-surface',
  },{ 'name': 'test-rng',
  }
]

if gegl.found()
  tests += {
      'name': 'test-gegl-surface',
      'srcs': 'gegl/test-gegl-surface.c',
      'deps': [ gegl, ],
      'incs': [ libmypaint_gegl_inc, ],
      'link': [ libmypaint_gegl ],
  }
endif

libmypaint_tests_lib = static_library('mypaint-tests',
  'mypaint-benchmark.c',
  'mypaint-test-surface.c',
  'mypaint-utils-stroke-player.c',
  'testutils.c',
  brush_settings_headers,
  c_args: [
    '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.source_root()),
  ],
  include_directories: toplevel_inc,
  dependencies: [
    glib,
  ],
)

foreach test: tests
  test_name = test.get('name')
  test_srcs = test.get('srcs', test_name + '.c')
  test_deps = test.get('deps', [])
  test_incs = test.get('incs', [])
  test_link = test.get('link', [])

  test_exe = executable(test_name,
    test_srcs, brush_settings_headers,
    c_args: '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.source_root()),
    include_directories: [ toplevel_inc, test_incs ],
    link_with: [
      libmypaint,
      libmypaint_tests_lib,
      test_link,
    ],
    dependencies: [
      glib,
      libmath,
      libprofiler,
      test_deps,
    ],
  )

  test(test_name, test_exe, timeout: 500)
endforeach
