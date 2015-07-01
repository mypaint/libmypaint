# Information for translators
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

    scons translate=pot

and then commit the modified `po/libmypaint.pot` too,
along with your changes.
Keeping this generated template file in the distribution
allows WebLate users to create new translations by themselves
without having to ask us.

## New translation (manual)

New translations can be started manually too.
Start by updating `po/libmypaint.pot` if necessary,
as described above,
then put a new `.po` file into this directory.

To make such a file you can
copy the header from an existing `.po` file
and modify it accordingly.

Unless there are several country-specific dialects for your language,
the file should be named simply `ll.po`
where "ll" is a recognized [language code][ll].

If there are several dialects for your language,
the file should be named `ll_CC.po`
where "CC" is the [country code][CC].

## Update translation (manual)

Before working on a translation,
update the `.po` file for your language.
For example, for the french translation, run:

    scons translate=fr

## Use/Test the translation

After modifying the translation,
you need to rebuild to see the changes:

    scons

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

Changes made in WebLate are asy for us to merge,
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
