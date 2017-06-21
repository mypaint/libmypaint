# API and ABI versioning policy

libMyPaint cannot afford to be sloppy about its version numbers because
other projects depend on it.

API versions on `master` increment ahead of the upcoming release
as and when features and changes are added. ABI versions are always
updated immediately before each release, and _only_ then.

1. The `master` branch contains the _future release_'s API version
   number. This version number is updated as and when new features and
   API changes are committed to the master branch. Do this by updating
   [configure.ac][]'s `libmypaint_api_*` macros as part of your commit.
   The reference point for these updates is the current stable release's
   version number.

   * Rules for the API version number: libmypaint uses
     [Semantic Versioning][].

   * Note that the major and minor components of the API version number
     form part of the _soname_ for binary library files used at runtime,
     but they're not part of the `libmypaint.so` symlink used by your
     compiler when it tries to find a dynamic library to link against.

2. We use prerelease suffixes at the end of the API version number
   during active development to help you identify what you are building
   against. The API version number of a formal release normally does not
   contain any prerelease sufffix.

3. For ABI versioning, libmypaint does exactly what the GNU docs say. We
   will always update some part of the ABI version number immediately
   before each public release. This is done by tweaking
   [configure.ac][]'s `libmypaint_abi_*` macros. ABI versions are _only_
   bumped immediately before a release, and maintain their own (somewhat
   weird) version sequence that's independent of the API version number.

   * Rules for the ABI version number: refer to
     “[Updating library version information][]”
     in the GNU libtool manual.

[configure.ac]: ./configure.ac
[Semantic Versioning]: http://semver.org/
[Updating library version information]: https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
