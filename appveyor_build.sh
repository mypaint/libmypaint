#!/usr/bin/env bash

# Exit early if any command fails
set -e

PKG_PREFIX="mingw-w64-$MSYS2_ARCH"

# Install dependencies
echo "Prefix: $PKG_PREF"
pacman --noconfirm -S --needed \
       base-devel \
       ${PKG_PREFIX}-json-c \
       ${PKG_PREFIX}-glib2 \
       ${PKG_PREFIX}-gobject-introspection


# Add m4 directories to the ACLOCAL_PATH
# Remove this if/when the underlying reason for their omission is found
for p in $(pacman --noconfirm -Ql ${PKG_PREFIX}-glib2 | # List files
		  grep "\.m4" | # We only care about the macro files
		  xargs readlink -e | # Make canonical, just in case
		  xargs dirname | # Strip file names from paths
		  sort --unique # Add each dir only once
	  )
do
    echo "Adding additional m4 path: $p"
    ACLOCAL_PATH=$p:$ACLOCAL_PATH
done

export ACLOCAL_PATH
export PWD="$APPVEYOR_BULD_FOLDER"

./autogen.sh
./configure
make distcheck
