# Migration from Autotools to CMake

This document describes the removal of the Autotools build system from pupnp
and what has changed for users, contributors, and the release process.

## Background

pupnp historically supported two parallel build systems: Autotools
(`configure` / `make`) and CMake.  Maintaining both required duplicating build
logic and keeping version numbers in sync across `configure.ac` and
`CMakeLists.txt`.  As of this migration, Autotools has been removed and CMake
is the sole build system.

## What was removed

| File / directory | Purpose |
|------------------|---------|
| `configure.ac` | Top-level Autotools configuration (version, feature flags) |
| `bootstrap` | Script to generate `configure` from `configure.ac` |
| `Makefile.am` (×6) | Autotools makefiles for the top level, ixml, upnp, samples, samples/tv, docs |
| `m4/` | Autotools macro files (ax_pthread, ax_cflags_*, etc.) |
| `cmake/autoheader.cmake` | CMake module that parsed `configure.ac` to derive version numbers |
| `scripts/comp-autotools.sh` | Developer helper script for Autotools build and test cycle |

## What changed

### Version numbers

Previously the authoritative version was in `configure.ac` (the `AC_INIT`
line) and `cmake/autoheader.cmake` parsed it at configure time.

Now the version is defined directly in `CMakeLists.txt`, as the single
`project(PUPNP ...)` version:

```cmake
project(PUPNP
	VERSION 22.0.3
	LANGUAGES C)
```

ixml is built and installed as a component of the `UPNP` package
(`UPNP::IXMLShared` / `UPNP::IXMLStatic`) rather than as a separately
versioned library. Both `libupnp` and `libixml` share this single version
number and `SOVERSION`; there is no independent ixml version and no
`find_package(IXML)`. An earlier iteration of this migration gave ixml and
upnp independent versions and derived a third "umbrella" project version by
adding them together — that scheme has been replaced by this single version
to keep the two co-released libraries from drifting apart or producing a
project version that doesn't correspond to either library's own history.

### `autoconfig.h`

`cmake/autoheader.cmake` generated `autoconfig.h.cm` at configure time by
parsing `configure.ac`.  It has been replaced by a static template
`cmake/autoconfig.h.in`, which CMake processes directly via
`configure_file()`.  The set of defines is unchanged.

### Source tarball (`make distcheck` → CPack)

`make distcheck` was the Autotools way to produce a source tarball and verify
the build from it.  The equivalent is now:

```sh
cmake -B build
cpack -G TBZ2 --config build/CPackSourceConfig.cmake
```

This produces `libupnp-X.Y.Z.tar.bz2` in the repository root, identical in
content to the old `make dist` tarball.  The CPack configuration lives at the
bottom of the top-level `CMakeLists.txt`.

### Release workflow (`.github/workflows/release.yml`)

The Configure and Build steps in the release workflow changed:

| Before | After |
|--------|-------|
| `./bootstrap && ./configure` | `cmake -B build` |
| `make distcheck` | `cpack -G TBZ2 --config build/CPackSourceConfig.cmake` |
| `git add … configure.ac` | `git add … CMakeLists.txt` |

### Home-keeping script (`scripts/home_keeping.sh`)

After each release this script prepares the tree for the next development
cycle.  Previously it incremented the libtool revision numbers inside
`configure.ac`.  Now it increments the patch component of the single
`project(PUPNP VERSION X.Y.Z ...)` line in `CMakeLists.txt`:

```sh
# Old: complex parsing of AC_SUBST([LT_VERSION_*]) lines in configure.ac
# New: locate the "VERSION X.Y.Z" line inside project(PUPNP ...) and bump the patch
version_line=$(grep -n -E '^\s*VERSION [0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt | head -1 | cut -d: -f1)
sed -i -E "${version_line}s/[0-9]+\.[0-9]+\.[0-9]+/${new_version}/" CMakeLists.txt
```

### CI workflows

The sanitizer build job in `.github/workflows/ccpp.yml` previously ran a
matrix of two tools (`autotools` and `cmake`).  The `autotools` leg has been
removed; the job now runs the CMake leg only.

The `sanitizer_autotools_flags` environment variable in
`.github/workflows/omnios.yml` has also been removed.

## Building (unchanged for CMake users)

Nothing changes for anyone already using CMake:

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build
```

## Testing the release workflow locally

`scripts/test_release_workflow.sh` simulates the full GitHub Actions release
workflow locally.  Run it from the repository root to catch regressions in the
release scripts or CPack configuration before pushing:

```sh
./scripts/test_release_workflow.sh
```

The script derives the current version from `CMakeLists.txt`, runs all six
release steps on a temporary git branch, and cleans up all artifacts on exit.
