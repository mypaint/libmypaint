## libmypaint - MyPaint brush engine library

[![Translation Status](https://hosted.weblate.org/widgets/mypaint/libmypaint/svg-badge.svg)](https://hosted.weblate.org/engage/mypaint/?utm_source=widget)
[![Travis Build Status](https://travis-ci.org/mypaint/libmypaint.png?branch=master)](https://travis-ci.org/mypaint/libmypaint)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/vc6ejt4nba5ctd6r)](https://ci.appveyor.com/project/jonnor/libmypaint)

This is a self-contained C library that is isolated from MyPaint.
It allows other applications to make use of the MyPaint brush engine.

License: ISC, see [COPYING](./COPYING) for details

### Prerequisites

Build dependencies:

* [json-c](https://github.com/json-c/json-c/wiki) (>= 0.11)
* [SCons](http://scons.org/)
* [Python](http://python.org/)

Optional dependencies:

* [GEGL + BABL](http://gegl.org/) (for enable_gegl=true)
* [GObjectIntrospection](https://live.gnome.org/GObjectIntrospection) (for enable_introspection=true)

### Building

To build for a given prefix, but not install any files yet:

    $ scons prefix=/tmp/testprefix1
    $ scons prefix=/tmp/testprefix1 .

These two lines are equivalent.
SCons's default target location is `.` (the current working directory).

To build and install libmypaint somewhere else,
you will need to supply a different target location.
For the simplest case, the target is the same as the prefix.

    $ scons prefix=/tmp/testprefix1 install
    $ scons prefix=/tmp/testprefix1 /tmp/testprefix1

These two lines are equivalent too. To save time,
`install` is an alias for the current value of the `prefix` build variable.

Packagers often need the ability to build for a given prefix,
but have to install somewhere else.
We recommend using the sandboxing options built into SCons for this.
An example, with some additional build variables for flavour:

    $ scons enable_gegl=true use_sharedlib=yes prefix=/usr \
        --install-sandbox=/tmp/sandbox1 /tmp/sandbox1

This builds libmypaint as if its eventual prefix will be `/usr`,
but puts the files into `/tmp/sandbox1/usr`
as if the sandbox was the root of the filesystem.
Note that you have to repeat the sandbox location in this case.

You can also build a minimal version of libmypaint
directly into your application by including "libmypaint.c".

### Build options

To list out libmypaint's build variables, with help texts, run

    $ scons --help

### Documentation

Further documentation can be found in the MyPaint wiki:
<https://github.com/mypaint/mypaint/wiki/Brushlib>.