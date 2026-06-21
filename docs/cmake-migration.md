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

Now the version is defined directly in `CMakeLists.txt`:

```cmake
# Source of Truth for IXML
set(IXML_VERSION_MAJOR 11)
set(IXML_VERSION_MINOR 1)
set(IXML_VERSION_PATCH 8)

# Source of Truth for UPNP
set(UPNP_VERSION_MAJOR 22)
set(UPNP_VERSION_MINOR 0)
set(UPNP_VERSION_PATCH 3)

# Umbrella version (used for the tarball name and project() version)
math(EXPR PUPNP_VERSION_MAJOR "${IXML_VERSION_MAJOR} + ${UPNP_VERSION_MAJOR}")
math(EXPR PUPNP_VERSION_MINOR "${IXML_VERSION_MINOR} + ${UPNP_VERSION_MINOR}")
math(EXPR PUPNP_VERSION_PATCH "${IXML_VERSION_PATCH} + ${UPNP_VERSION_PATCH}")
```

The umbrella `PUPNP_VERSION_STRING` (e.g. `33.1.11`) is the sum of the IXML
and UPNP component versions, matching the scheme that was already in use.

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
`configure.ac`.  Now it increments `UPNP_VERSION_PATCH` in `CMakeLists.txt`:

```sh
# Old: complex parsing of AC_SUBST([LT_VERSION_*]) lines in configure.ac
# New: one grep + one sed on CMakeLists.txt
upnp_patch_line=$(grep -n "set(UPNP_VERSION_PATCH " CMakeLists.txt | cut -d: -f1)
sed -i -E "${upnp_patch_line}s/(set\(UPNP_VERSION_PATCH )([0-9]+)(\))/\1$((patch+1))\3/" CMakeLists.txt
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
