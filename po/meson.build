i18n = import('i18n')

i18n.gettext(
  gettext_package,
  args: [
    # Extract translations from generated file.
    # Unfortunately, there is currently no way to express the dependency
    # so one will need to build the project before updating the po template.
    # https://github.com/mesonbuild/meson/issues/1733
    brush_settings_headers[1].full_path(),
    # Avoid POTFILES.in since that uses intltool syntax incompatible with xgettext
    # and lists files that do not exist in the source directory.
    '--files-from=' + (meson.current_source_dir() / 'POTFILES.meson'),
    # Keep old file name for backwards compatibility.
    '--output=libmypaint.pot',
    # Unlike intltool-extract, xgettext does not support overriding location
    # with a comment in the generated file – it will preserve it as a regular
    # comment. Let’s not include the location in the generated file
    # since it is less useful.
    '--no-location',
  ],
  preset: 'glib',
)
