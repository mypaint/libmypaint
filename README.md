libmypaint - MyPaint brush engine library
===========================================
[![Travis Build Status](https://travis-ci.org/mypaint/libmypaint.png?branch=master)](https://travis-ci.org/mypaint/libmypaint)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/vc6ejt4nba5ctd6r)](https://ci.appveyor.com/project/jonnor/libmypaint)

This is a self-contained C library that is isolated from MyPaint.
It allows other applications to make use of the MyPaint brush engine.

License: ICS, see [COPYING](./COPYING) for details

Prerequisites
---------------

Build dependencies:
* [json-c](https://github.com/json-c/json-c/wiki) (>= 0.11)
* [scons](http://scons.org/)
* [Python](http://python.org/)

Optional dependencies:
* [GEGL + BABL](http://gegl.org/) (for enable_gegl=true)
* [GObjectIntrospection](https://live.gnome.org/GObjectIntrospection) (for enable_introspection=true)


Building
---------

    # Normal build
    scons prefix=/your/application/install/prefix

    # Show build options
    scons -h


You can also build a minimal version of libmypaint directly into your application by including "libmypaint.c"

Documentation
--------------

Documentation can be found in the wiki:
http://wiki.mypaint.info/Brushlib

