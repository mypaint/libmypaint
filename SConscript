from __future__ import print_function

Import('env', 'install_perms')

import os, sys
import subprocess

brushlib_version = '1.1'

def add_gobject_introspection(env, gi_name, version,
                              func_prefix, type_prefix,
                              sources, includepaths, library,
                              pkgs, includes):

    pkgs = ' '.join('--pkg=%s' % dep for dep in pkgs)
    library = library[0] # there should be only one Node in the list

    # Strip the library path to get the library name
    libname = os.path.basename(library.get_path())
    libname = os.path.splitext(libname)[0]
    if libname.startswith('lib'):
        libname = libname[3:]

    includeflags = ' '.join(['-I%s' % s for s in includepaths])
    gi_includes = ' '.join(['--include=%s' % s for s in includes])

    scanner_cmd = """LD_LIBRARY_PATH=./ g-ir-scanner -o $TARGET --warn-all \
        --namespace=%(gi_name)s --nsversion=%(version)s --add-include-path=./ \
        --identifier-prefix=%(type_prefix)s --symbol-prefix=%(func_prefix)s \
        %(pkgs)s %(includeflags)s %(gi_includes)s \
        --library=%(libname)s $SOURCES""" % locals()

    gir_file = env.Command("%s-%s.gir" % (gi_name, version), sources, scanner_cmd)
    env.Depends(gir_file, library)
    typelib_file = env.Command("%s-%s.typelib" % (gi_name, version), gir_file,
                           "g-ir-compiler --includedir=./ -o $TARGET $SOURCE")

    return (gir_file, typelib_file)

# Generate a @pkgconfig_name.pc and a @pkgconfig_name-uninstalled.pc
def create_pkgconfig_files(env, pkgconfig_name, version, description,
                           libname, deps, libs=[], linkflags=[], cflags=[]):

    pkg_info = {
        '@LIBNAME@': libname,
        '@REQUIRES@': ' '.join(deps),
        '@DESCRIPTION@': description,
        '@VERSION@': version,
        '@LIBS@': ' '.join('-l'+lib for lib in libs),
        '@LINKFLAGS@': ' '.join(linkflags),
        '@PREFIX@': env['prefix'],
        '@LIBDIR@': os.path.join(env['prefix'], 'lib'),
        '@INCLUDEDIR@': os.path.join(env['prefix'], 'include'),
    }
    pc_file = env.Substfile('%s.pc' % pkgconfig_name,
                            "pkgconfig.pc.in", SUBST_DICT=pkg_info)
    install_perms(env, '$prefix/lib/pkgconfig', pc_file)

    return pc_file


# Helper for ParseConfig

def parse_pkg_config(env, libname):
    extras = ""
    if sys.platform == "msys" and os.environ.get("MSYSTEM") != "MSYS":
        # Building on e.g. MINGW32 using MSYS scons.
        # Paths will have drive letters & mess up scons's os.path.join()s
        # unless we do this:
        extras += " --dont-define-prefix"
    cmd = "pkg-config --cflags --libs {libname} {extras}".format(
        libname = libname,
        extras = extras,
    )
    env.ParseConfig(cmd)


# NOTE: We use a copy of the environment, to be able to both inherit common options,
# and also add our own specifics ones without affecting the other builds
top_env = env
env = env.Clone()

if env['enable_introspection']:
    if not env['use_glib']:
        print("Enabling glib because of enable_introspection=true")
    if not env['use_sharedlib']:
        print("Building a shared lib instead of a static lib because of enable_introspection=true")
    env['use_glib'] = True
    env['use_sharedlib'] = True

Export('env')

if env['enable_docs']:
    doc = SConscript('doc/SConscript')

env.Append(CPPPATH='./')

env.Append(CPPDEFINES='HAVE_JSON_C')
json_pkgconfig = 'json-c'

if subprocess.call(["pkg-config", json_pkgconfig]):
    print("Could not find 'json-c' pkg-config, trying legacy 'json' instead")
    json_pkgconfig = 'json'

pkg_deps = [json_pkgconfig]
libs = ['m']
linkflags = []

if env['enable_openmp']:
    env.Append(CFLAGS='-fopenmp')
    linkflags += ['-fopenmp']

if env['enable_i18n']:
    env.Append(CPPDEFINES='HAVE_GETTEXT')
    if not sys.platform.startswith("gnu") and not sys.platform.startswith("linux"):
        libs += ['intl']


config_defines = ''
if env['use_glib']:
    pkg_deps += ['gobject-2.0']
    config_defines += '#define MYPAINT_CONFIG_USE_GLIB 1\n'
else:
    config_defines += '#define MYPAINT_CONFIG_USE_GLIB 0\n'

config_file = env.Substfile("mypaint-config.h", "mypaint-config.h.in",
                            SUBST_DICT={'@DEFINES@': config_defines})

def generate_cheaders(env, target, source):
    cmd = ' '.join([
        env['python_binary'],
        str(source[0]),
        str(target[0]),
        str(target[1]),
    ])
    env.Execute(cmd)

env.Command(['mypaint-brush-settings-gen.h', 'brushsettings-gen.h'],
            ['generate.py', 'brushsettings.py', 'brushsettings.json'],
            generate_cheaders)

env.Clean('.', Glob('*-gen.h'))
env.Clean('.', Glob('*.pyc'))
env.Clean('.', Glob('*.o'))

env.Append(LINKFLAGS=linkflags)
env.Append(LIBS=libs)

for pkg in pkg_deps:
    parse_pkg_config(env, pkg)

lib_builder = env.SharedLibrary if env['use_sharedlib'] else env.StaticPicLibrary
sources = Glob("*.c")
sources = [n for n in sources if not n.name == "libmypaint.c"]
brushlib = lib_builder('./mypaint', sources)

create_pkgconfig_files(env, 'libmypaint', brushlib_version, 'MyPaint brush engine library',
                       libname='mypaint', deps=pkg_deps, libs=libs, linkflags=linkflags)

if env['enable_introspection']:
    gir, typelib = add_gobject_introspection(env, "MyPaint", brushlib_version,
                              "mypaint_", "MyPaint",
                              Glob("*.c") + Glob("mypaint-*.h") +
                              Glob("glib/mypaint-[!gegl]*.c") + Glob("glib/mypaint-[!gegl]*.h"),
                              ['./'], brushlib, ['glib-2.0'], [])

    install_perms(env, '$prefix/share/gir-1.0', gir)
    install_perms(env, '$prefix/lib/girepository-1.0', typelib)

install_perms(env, '$prefix/lib/', brushlib)
install_perms(env, '$prefix/include/libmypaint', Glob("./mypaint-*.h"))
install_perms(env, '$prefix/include/libmypaint/glib', Glob("./glib/mypaint-*.h"))
install_perms(env, "$prefix/share/libmypaint", Glob("./*.py"))
install_perms(env, "$prefix/share/libmypaint", "./brushsettings.json")

if env['enable_i18n']:
    languages = SConscript('po/SConscript')

# Optional: GEGL library
gegl_env = env.Clone()
if env['enable_gegl']:
    deps = ['gegl-%s' % env['GEGL_VERSION']]
    for d in deps:
        parse_pkg_config(gegl_env, d)
    gegl_env.Append(LIBPATH=['.'], LIBS=['mypaint'])

    lib_builder = gegl_env.SharedLibrary if env['use_sharedlib'] else gegl_env.StaticPicLibrary
    brushlib_gegl = lib_builder('./mypaint-gegl', Glob("./gegl/*.c"))

    install_perms(env, '$prefix/lib/', brushlib_gegl)
    install_perms(env, '$prefix/include/libmypaint-gegl', Glob("./gegl/mypaint-gegl-*.h"))

    create_pkgconfig_files(env, 'libmypaint-gegl', brushlib_version, 'MyPaint brush engine library, with GEGL integration',
                           libname='mypaint-gegl', deps=deps + ['libmypaint'])

    if gegl_env['enable_introspection']:
        gir, typelib = add_gobject_introspection(gegl_env, "MyPaintGegl", brushlib_version,
                                  "mypaint_gegl", "MyPaintGegl",
                                  Glob("gegl/*.c") + Glob("gegl/mypaint-gegl-*.h") +
                                  Glob("glib/mypaint-gegl*"),
                                  ['./', './gegl'], brushlib_gegl,
                                  ['glib-2.0', 'gegl-%s' % env['GEGL_VERSION']],
                                  ['Gegl-%s' % env['GEGL_VERSION'], 'MyPaint-%s' % brushlib_version])

        install_perms(env, '$prefix/share/gir-1.0', gir)
        install_perms(env, '$prefix/lib/girepository-1.0', typelib)

Export('gegl_env', 'env', 'parse_pkg_config')
tests = SConscript('tests/SConscript')

Return('brushlib')

# vim:syntax=python
