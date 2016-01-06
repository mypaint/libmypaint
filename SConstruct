import os, sys
from os.path import join, basename
from SCons.Script.SConscript import SConsEnvironment
import SCons.Util

EnsureSConsVersion(1, 0)

SConsignFile() # no .scsonsign into $PREFIX please

default_openmp = True
default_prefix = '/usr/local/'

if sys.platform == "darwin":
    default_openmp = False

default_python_binary = 'python%d.%d' % (sys.version_info[0], sys.version_info[1])

def isabs(key, dirname, env):
    assert os.path.isabs(dirname), "%r must have absolute path syntax" % (key,)

opts = Variables()
opts.Add(
    'prefix', 'autotools-style installation prefix',
    default=default_prefix,
    validator=isabs,
)
opts.Add('libdir', 'Directory into which libraries are installed', default='lib')
opts.Add('includedir', 'Directory into which headers are installed', default='include')
opts.Add('datadir', 'Directory into which shared data is installed', default='share')
opts.Add(BoolVariable('debug', 'enable HEAVY_DEBUG and disable optimizations', False))
opts.Add(BoolVariable('enable_profiling', 'enable debug symbols for profiling purposes', True))
opts.Add(BoolVariable('enable_i18n', 'enable i18n support for brushlib (requires gettext)', True))
opts.Add(BoolVariable('enable_gegl', 'enable GEGL based code in build', False))
opts.Add(BoolVariable('enable_introspection', 'enable GObject introspection support', False))
opts.Add(BoolVariable('enable_docs', 'enable documentation build', False))
opts.Add(BoolVariable('enable_gperftools', 'enable gperftools in build, for profiling', False))
opts.Add(BoolVariable('enable_openmp', 'enable OpenMP for multithreaded processing', default_openmp))
opts.Add(BoolVariable('use_sharedlib', 'build a shared library instead of a static library (forced on by introspection)', True))
opts.Add(BoolVariable('use_glib', 'enable glib (forced on by introspection)', False))
opts.Add('python_binary', 'python executable to build for', default_python_binary)

tools = ['default', 'textfile']

env = Environment(ENV=os.environ, options=opts, tools=tools)

Help(opts.GenerateHelpText(env))

# Respect some standard build environment stuff
# See http://cgit.freedesktop.org/mesa/mesa/tree/scons/gallium.py
# See https://wiki.gentoo.org/wiki/SCons#Missing_CC.2C_CFLAGS.2C_LDFLAGS
if os.environ.has_key('CC'):
   env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
   env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('CPPFLAGS'):
   env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CPPFLAGS'])
if os.environ.has_key('LDFLAGS'):
    # LDFLAGS is omitted in SHLINKFLAGS, which is derived from LINKFLAGS
   env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

opts.Update(env)

for d in ('lib', 'include', 'data'):
    key = d + 'dir'
    val = env[key]
    if not val.startswith(os.sep):
        env[key] = os.path.join(env['prefix'], val)

env.Append(CXXFLAGS=' -Wall -Wno-sign-compare -Wno-write-strings')
env.Append(CCFLAGS='-Wall -Wstrict-prototypes -Werror')
env.Append(CFLAGS='-std=c99')

env['GEGL_VERSION'] = 0.3

# Define strdup() in string.h under glibc >= 2.10 (POSIX.1-2008)
env.Append(CFLAGS='-D_POSIX_C_SOURCE=200809L')

if env.get('CPPDEFINES'):
    # make sure assertions are enabled
    env['CPPDEFINES'].remove('NDEBUG')

if env['debug']:
    env.Append(CPPDEFINES='HEAVY_DEBUG')
    env.Append(CCFLAGS='-O0', LINKFLAGS='-O0')
else:
    # Overridable defaults
    env.Prepend(CCFLAGS='-O3', LINKFLAGS='-O3')

if env['enable_profiling'] or env['debug']:
    env.Append(CCFLAGS='-g')

set_dir_postaction = {}
def install_perms(env, target, sources, perms=0644, dirperms=0755):
    """As a normal env.Install, but with Chmod postactions.

    The `target` parameter must be a string starting with ``$prefix``.
    The permissions in `perms` will be assigned to each file which was
    installed from `sources` in a post-install action.

    The `dirperms` permissions will be assigned to each created
    directory component which does not exist (at the time of calling
    this function). Each set of permission bits is assigned in its own
    postaction.

    """
    assert any(target.startswith(x) for x in (
        "$prefix", "$libdir", "$includedir", "$datadir"))
    install_targs = env.Install(target, sources)

    # Set file permissions, and defer directory permission setting
    for targ in install_targs:
        env.AddPostAction(targ, Chmod(targ, perms))
        d = os.path.dirname(os.path.normpath(targ.get_abspath()))
        d_prev = None
        while d != d_prev and not os.path.exists(d):
            if not d in set_dir_postaction:
                env.AddPostAction(targ, Chmod(d, dirperms))
                set_dir_postaction[d] = True
            d_prev = d
            d = os.path.dirname(d)

    # Return like Install()
    return install_targs


def install_tree(env, dest, path, perms=0644, dirperms=0755):
    assert os.path.isdir(path)
    target_root = join(dest, os.path.basename(path))
    for dirpath, dirnames, filenames in os.walk(path):
        reltarg = os.path.relpath(dirpath, path)
        target_dir = join(target_root, reltarg)
        target_dir = os.path.normpath(target_dir)
        filepaths = [join(dirpath, basename) for basename in filenames]
        install_perms(env, target_dir, filepaths, perms=perms, dirperms=dirperms)

def createStaticPicLibraryBuilder(env):
    """This is a utility function that creates the StaticExtLibrary Builder in
    an Environment if it is not there already.

    If it is already there, we return the existing one."""
    import SCons.Action

    try:
        static_extlib = env['BUILDERS']['StaticPicLibrary']
    except KeyError:
        action_list = [ SCons.Action.Action("$ARCOM", "$ARCOMSTR") ]
        if env.Detect('ranlib'):
            ranlib_action = SCons.Action.Action("$RANLIBCOM", "$RANLIBCOMSTR")
            action_list.append(ranlib_action)

    static_extlib = SCons.Builder.Builder(action = action_list,
                                          emitter = '$LIBEMITTER',
                                          prefix = '$LIBPREFIX',
                                          suffix = '$LIBSUFFIX',
                                          src_suffix = '$OBJSUFFIX',
                                          src_builder = 'SharedObject')

    env['BUILDERS']['StaticPicLibrary'] = static_extlib
    return static_extlib

createStaticPicLibraryBuilder(env)


# Convenience alias for installing to $prefix
env.Alias('install', '$prefix')

Export('env', 'install_tree', 'install_perms')

brushlib = SConscript('./SConscript')
