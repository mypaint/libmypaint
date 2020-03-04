#!/usr/bin/env bash
1;5202;0csec()
{
    date -r "$1" "+%s"
}

ORIG=../brushsettings.json
SRC=../brushsettings-gen.h
ENUM=../mypaint-brush-settings-gen.h
GEN=../generate.py

# Check if the message source file needs to be (re)generated.
# ( generated source: not present,older than basis, older than script )
if [ ! -e "$SRC" -o \
       $(sec "$SRC") -lt $(sec "$ORIG") -o  \
       $(sec "$SRC") -lt $(sec "$GEN") ]
then
    echo "Generated file missing or out of date, generating..."
    python ../generate.py "$ENUM" "$SRC" ||
        (echo "Failed to generate source file!" && exit 1)
fi

# Omit locations from the generated file, and instead...
xgettext --no-location -c -kN_:1 -o - "$SRC" |
# ...transform special generated comments into accurate source locations.
    sed -E "s@^#\. (: $ORIG:.*)@#\1@" > libmypaint.pot
