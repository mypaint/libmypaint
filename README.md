# libmypaint - MyPaint brush engine library

[![Translation status](https://hosted.weblate.org/widgets/mypaint/-/libmypaint/svg-badge.svg)](https://hosted.weblate.org/engage/mypaint/?utm_source=widget)
[![Travis Build Status](https://travis-ci.org/mypaint/libmypaint.svg?branch=master)](https://travis-ci.org/mypaint/libmypaint)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/mypaint/libmypaint?branch=master&svg=true)](https://ci.appveyor.com/project/jonnor/libmypaint)

This is the brush library used by MyPaint. A number of other painting
programs use it too.

License: ISC, see [COPYING](./COPYING) for details.

## Dependencies

* All configurations and builds:
  - [json-c](https://github.com/json-c/json-c/wiki) (>= 0.11)
  - C compiler, `make` etc.
* Most configurations (all except `--disable-introspection --without-glib`):
  - [GObject-Introspection](https://live.gnome.org/GObjectIntrospection)
  - [GLib](https://wiki.gnome.org/Projects/GLib)
* When building from `git` (developer package names vary by distribution):
  - [Python](http://python.org/)
  - [autotools](https://en.wikipedia.org/wiki/GNU_Build_System)
  - [intltool](https://freedesktop.org/wiki/Software/intltool/)
  - [gettext](https://www.gnu.org/software/gettext/gettext.html)
* For `--enable-gegl` (GIMP *does not* require this):
  - [GEGL + BABL](http://gegl.org/)

### Install dependencies (Debian and derivatives)

On recent Debian-like systems, you can type the following
to get started with a standard configuration:

    # apt install -y build-essential
    # apt install -y libjson-c-dev libgirepository1.0-dev libglib2.0-dev

When building from git:

    # apt install -y python autotools-dev intltool gettext libtool
    
You might also try using your package manager:

    # apt build-dep mypaint # will get additional deps for MyPaint (GUI)
    # apt build-dep libmypaint  # may not exist; included in mypaint

### Install dependencies (Red Hat and derivatives)

The following works on a minimal CentOS 7 installation:

    # yum install -y gcc gobject-introspection-devel json-c-devel glib2-devel

When building from git, you'll want to add:

    # yum install -y git python autoconf intltool gettext libtool
    
You might also try your package manager:

    # yum builddep libmypaint

### Install dependencies (OpenSUSE)

Works with a fresh OpenSUSE Tumbleweed Docker image:

    # zypper install gcc13 gobject-introspection-devel libjson-c-devel glib2-devel

When building from git:

    # zypper install git python311 autoconf intltool gettext-tools libtool

Package manager:

    # zypper install libmypaint0

## Build and install

MyPaint and libmypaint benefit dramatically from autovectorization and other compiler optimizations.
You may want to set your CFLAGS before compiling (for gcc):

    $ export CFLAGS='-Ofast -ftree-vectorize -fopt-info-vec-optimized -march=native -mtune=native -funsafe-math-optimizations -funsafe-loop-optimizations'

The traditional setup works just fine.

    $ ./autogen.sh    # Only needed when building from git.
    $ ./configure
    # make install
    # ldconfig

### Maintainer mode

We don't ship a `configure` script in our git repository. If you're
building from git, you have to kickstart the build environment with:

    $ git clone https://github.com/mypaint/libmypaint.git
    $ cd libmypaint
    $ ./autogen.sh

This script generates `configure` from `configure.ac`, after running a
few checks to make sure your build environment is broadly OK. It also
regenerates certain important generated headers if they need it.

Folks building from a release tarball don't need to do this: they will
have a `configure` script from the start.

### Configure

    $ ./configure
    $ ./configure --prefix=/tmp/junk/example

There are several MyPaint-specific options.
These can be shown by running

    $ ./configure --help

### Build

    $ make

Once MyPaint is built, you can run the test suite and/or install it.

### Test

    $ make check

This runs all the unit tests.

### Install

    # make install

Uninstall libmypaint with `make uninstall`.

### Check availability

Make sure that pkg-config can see libmypaint before trying to build with it.

    $ pkg-config --list-all | grep -i mypaint

If it's not found, you'll need to add the relevant pkgconfig directory to
the `pkg-config` search path. For example, on CentOS, with a default install:

    # sh -c "echo 'PKG_CONFIG_PATH=/usr/local/lib/pkgconfig' >>/etc/environment"

Make sure ldconfig can see libmypaint as well

    # ldconfig -p |grep -i libmypaint

If it's not found, you'll need to add the relevant lib directory to
the LD_LIBRARY_PATH:
    
    $ export LD_LIBRARY_PATH=/usr/local/lib
    # sh -c "echo 'LD_LIBRARY_PATH=/usr/local/lib' >>/etc/environment

Alternatively, you may want to enable /usr/local for libraries.  Arch and Redhat derivatives:

    # sh -c "echo '/usr/local/lib' > /etc/ld.so.conf.d/usrlocal.conf"
    # ldconfig

## Contributing

The MyPaint project welcomes and encourages participation by everyone.
We want our community to be skilled and diverse,
and we want it to be a community that anybody can feel good about joining.
No matter who you are or what your background is, we welcome you.

Please note that MyPaint is released with a
[Contributor Code of Conduct](CODE_OF_CONDUCT.md).
By participating in this project you agree to abide by its terms.

Please see the file [CONTRIBUTING.md](CONTRIBUTING.md)
for details of how you can begin contributing.

## Making releases

The distribution release can be generated with:

    $ make dist

And it should be checked before public release with:

    $ make distcheck

## Localization

Contribute translations here: <https://hosted.weblate.org/engage/mypaint/>.

The list of languages is maintained in [po/LINGUAS](po/LINGUAS).
Currently this file lists all the languages we have translations for.
It can be regenerated with:

    $ ls po/*.po | sed 's$^.*po/\([^.]*\).po$\1$' | sort > po/LINGUAS

You can also disable languages by removing them from the list if needed.

A list of files where localizable strings can be found is maintained
in `po/POTFILES.in`.

### Strings update

You can update the .po files when translated strings in the code change
using:

    $ cd po && make update-po

When the results of this are pushed, Weblate translators will see the
new strings immediately.

## Documentation

Further documentation can be found in the libmypaint wiki:
<https://github.com/mypaint/libmypaint/wiki>.


## Software using libmypaint

* [MyPaint](https://github.com/mypaint/mypaint)
* [GIMP](https://gitlab.gnome.org/GNOME/gimp/)
* [OpenToonz](https://github.com/opentoonz/opentoonz/)
* [enve](https://github.com/MaurycyLiebner/enve/)
