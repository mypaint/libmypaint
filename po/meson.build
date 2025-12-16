# Translations need to be extracted from a generated file.  Unfortunately,
# there is currently no way to express the dependency with `i18n.gettext()`,
# so one would need to explicitly rebuild the project before any action.
# https://github.com/mesonbuild/meson/issues/1733
# Additionally, we need to do some post-processing because xgettext
# would use the generated file path for location info.
#
# Let’s recreate the built-in targets created by `i18n.gettext()` ourselves.

fs = import('fs')

languages = fs.read('LINGUAS').split()

gettext_maintainer_tools = {
  'cp': cp.found(),
  'msginit': msginit.found(),
  'msgmerge': msgmerge.found(),
  'xgettext': xgettext.found(),
}

has_maintainer_tools = true
foreach tool, available : gettext_maintainer_tools
  if not available
    has_maintainer_tools = false
    warning(tool + ' not found, maintainer targets will not work')
  endif
endforeach

# Generate compiled message catalogues (.mo files).
foreach lang : languages
  message_dir = localedir / lang / 'LC_MESSAGES'
  mo_lang_name = f'@gettext_package@-@lang@.mo'

  custom_target(
    mo_lang_name,
    input: f'@lang@.po',
    # This should really be `@gettext_package@.mo` for every language,
    # and the languages should be distinguished by directory name
    # but Meson does not support output in a subdirectory.
    # https://github.com/mesonbuild/meson/issues/2320
    output: mo_lang_name,
    command: [
      msgfmt,
      '--output-file', '@OUTPUT@',
      '@INPUT@',
    ],
    install: true,
    install_dir: message_dir,
    install_tag: 'i18n',
  )

  # Since we have to use different names to disambiguate and there is no
  # `rename` kwarg, we need to rename it after installation.
  # This will break uninstall script but 🤷‍♀️
  meson.add_install_script(
    mv,
    message_dir / mo_lang_name,
    message_dir / f'@gettext_package@.mo',
  )
endforeach

if has_maintainer_tools
  update_po_targets = []

  # Update PO template.

  pot = custom_target(
    '_update-pot',
    input: brush_settings_headers[1],
    depends: [
      brush_settings_headers,
    ],
    command: [
      xgettext,
      '--package-name=' + gettext_package,
      '--output-dir=' + meson.project_build_root(),
      '--directory=' + meson.project_build_root(),
      '--output=@OUTPUT@',
      '--add-comments',
      '--keyword=N_:1',
      '@INPUT@',
      # The input is a generated file and our generator includes
      # the actual source location in a comment for translators.
      # We are going to convert it into a proper location comment
      # in the next step.
      '--no-location',
    ],
    output: 'libmypaint.pot.in',
  )

  pot = custom_target(
    '_fix-pot',
    input: pot,
    command: [
      # ...transform special generated comments into accurate source locations.
      fix_po_location,
      '@INPUT@',
      '@OUTPUT@',
    ],
    # Keep old file name for backwards compatibility.
    output: 'libmypaint.pot',
  )

  update_pot = run_target(
    'libmypaint.pot',
    command: [
      cp,
      pot,
      meson.current_source_dir(),
    ],
    depends: pot,
  )

  # Update PO files.

  foreach lang : languages
    pofile = meson.current_source_dir() / f'@lang@.po'
    update_po_target = f'update-po-@lang@'

    if fs.is_file(pofile)
      update_po_targets += run_target(
        update_po_target,
        command: [
          msgmerge,
          '--quiet',
          '--output-file', pofile,
          pofile,
          # `run_target` cannot depend on another `run_target`.
          # Use .pot file from build directory.
          pot,
        ],
        depends: [
          pot,
        ],
      )
    else
      update_po_targets += run_target(
        update_po_target,
        command: [
          msginit,
          # `run_target` cannot depend on another `run_target`.
          # Use .pot file from build directory.
          '--input=' + pot,
          '--output-file=' + pofile,
          '--locale=' + lang,
          '--no-translator',
        ],
        depends: [
          pot,
        ],
      )
    endif
  endforeach

  alias_target(
    f'update-po',
    update_pot,
    update_po_targets,
  )
endif
