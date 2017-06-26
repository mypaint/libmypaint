# Translating libmypaint

[![Translation status](https://hosted.weblate.org/widgets/mypaint/-/svg-badge.svg)](https://hosted.weblate.org/engage/mypaint/?utm_source=widget)

libmypaint uses WebLate for its translations.
You can get involved at <https://hosted.weblate.org/engage/mypaint/>.

That might be all the information you need
if you just want to help ensure that libmypaint
is correctly translated into your language.

# Information for coders and maintainers

We use [GNU gettext][gettext] for runtime translation of program text.

## After updating program strings

After changing any string in the source text which makes use of the
gettext macros, you will need to manually run

    cd po/
    intltool-update -g libmypaint --pot

    for lang in `cat LINGUAS`; do
      intltool-update -g libmypaint --dist $lang
    done

and then commit the modified `po/libmypaint.pot` and `po/*.po`
files along with your changes.
Keeping this generated template file in the distribution
allows WebLate users to create new translations by themselves
without having to ask us.

The `.pot` file alone can be updated by running
just the first `intltool-update` command,
if all you want to do is compare diffs.

# Information for translators

## New translation (manual)

New translations can be started manually too.
Start by putting a new stub `.po` file into this directory.

To make such a file you can
copy the header from an existing `.po` file
and modify it accordingly.

You must also add an entry to the `po/LINGUAS` file for libmypaint
so that message catalogs will be built. This can be done with

    cd po/
    ls *.po | sed 's/.po//' | sort >LINGUAS

Unless there are several country-specific dialects for your language,
the file should be named simply `ll.po`
where "ll" is a recognized [language code][ll].

If there are several dialects for your language,
the file should be named `ll_CC.po`
where "CC" is the [country code][CC].

Before you can work on this `.po` file,
you will need to update it.

## Update translation (manual)

Before working on a translation,
update the `.po` file for your language.
For example, for the French translation, run:

    intltool-update -g libmypaint --dist fr

## Use/Test the translation

After modifying the translation,
you need to rebuild and reinstall libmypaint to see the effect
in MyPaint, or in other apps that use `libmypaint`.
If you have already built it, and `make install` does what you want,
you can just use:

    make
    make install

To run MyPaint with a specific translation on Linux,
you can use the LANG environment variable
like this (the locale needs to be supported):

    LANG=ll_CC.utf8 ./mypaint

where "ll" is a [language code][ll], and and "CC" is a [country code][CC].
Your working directory must be the root directory of the mypaint source.

To run MyPaint with the original strings, for comparison,
you can use the `LC_MESSAGES` variable like this:

    LC_MESSAGES=C ./mypaint

## Send changes (manual)

Before you send your changes, please make sure that
your changes are based on the
current development (git) version of libmypaint.

Changes made in WebLate are easy for us to merge,
but changes sent as [Github pull requests][PR] are fine too.
If you do not know git just send
either a unified diff or the updated .po file
along with your name to: *a.t.chadwick (AT) gmail.com*.

If you are interested in keeping the transalations up to date,
please subscribe to the MyPaint project on WebLate:
<https://hosted.weblate.org/accounts/profile/#subscriptions>.

--------------------

[gettext]: http://www.gnu.org/software/hello/manual/gettext/ (Official GNU gettext manual)
[ll]: http://www.gnu.org/software/hello/manual/gettext/Usual-Language-Codes.html#Usual-Language-Codes ("ll" options)
[CC]: http://www.gnu.org/software/hello/manual/gettext/Country-Codes.html#Country-Codes ("CC" options)
[PR]: https://help.github.com/articles/using-pull-requests/
