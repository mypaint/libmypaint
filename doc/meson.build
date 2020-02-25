doc_conf = configuration_data()
doc_conf.merge_from(conf)

doc_conf.set('SOURCE_ROOT', meson.source_root())
doc_conf.set('BUILD_PATH', meson.current_build_dir())

doxyfile = configure_file(
  input: 'Doxyfile.in',
  output: 'Doxyfile',
  configuration: doc_conf,
)

doxygen_index = custom_target('doxygen',
  input : doxyfile,
  output: 'index.xml'
  command: [
    doxygen, '@INPUT@',
  ],
)

subdir('source')

run_target('sphinx',
  depends: doxygen_index,
  command: [
    sphinx_build,
    '-c', meson.current_build_dir() / 'source',
    meson.current_source_dir() / 'source',
    meson.current_build_dir() / 'build',
  ],
)
