if ENABLE_GEGL

TESTS = [
  'test-gegl-surface',
]

foreach name : TESTS
  e = executable(name,
    name + '.c',
    c_args : '-DLIBMYPAINT_TESTING_ABS_TOP_SRCDIR="@0@"'.format(meson.source_root()),
    include_directories : libmypaint_gegl_inc,
    link_with : [libmypaint_gegl, libmypaint],
    dependencies : [libmypaint_tests, GEGL, GLIB, MATH, LIBPROFILER],
  )
  test(name, e)
endforeach

endif
