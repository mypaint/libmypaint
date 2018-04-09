if ENABLE_DOCS
  doc_conf = configuration_data()
  doc_conf.merge_from(conf)
  doc_conf.set('SOURCE_ROOT', meson.source_root())
  doc_conf.set('BUILD_PATH', meson.current_build_dir())

  doxyfile = configure_file(
    input : 'Doxyfile.in',
    output : 'Doxyfile',
    configuration : doc_conf)

  doxygen_index = custom_target('doxygen',
    command : [DOXYGEN, '@INPUT@'],
    input : doxyfile,
    output : 'index.xml')

  subdir('source')

  run_target('sphinx',
    command : [SPHINX_BUILD,
      '-c', join_paths(meson.current_build_dir(), 'source'),
      join_paths(meson.current_source_dir(), 'source'),
      join_paths(meson.current_build_dir(), 'build')],
    depends : [doxygen_index])
endif
