#!/bin/sh
# regression: issue #365 – UpnpPrintf must not appear in the shared-library
# dynamic symbol table.
#
# Background
# ----------
# CMake builds compile the library with -fvisibility=hidden, so every symbol
# is hidden by default; only symbols marked UPNP_EXPORT_SPEC (which expands to
# __attribute__((visibility("default")))) are exported.  UpnpPrintf has no
# UPNP_EXPORT_SPEC, so it is already hidden in CMake builds – this test acts
# as a regression guard there.
#
# Autotools builds do NOT set -fvisibility=hidden globally; instead libtool
# uses -export-symbols-regex '^Upnp.*' to build a linker version script that
# exports every symbol whose name starts with "Upnp".  UpnpPrintf matches that
# regex, so without an explicit __attribute__((visibility("hidden"))) on its
# definition (upnpdebug.c) the symbol leaks into the public DSO interface.
# This test catches that leak – it is expected to FAIL on unfixed autotools
# builds and PASS once the visibility attribute is in place.
#
# Usage: $0 [<path-to-libupnp.so>]
#   With argument : CMake passes $<TARGET_FILE:upnp_shared>
#   No argument   : autotools – the script searches .libs/ relative to the
#                   upnp/ build directory where make check runs.
#
# Exit codes:  0 = PASS  1 = FAIL  77 = SKIP (nm absent / non-ELF platform)

SYMBOL="UpnpPrintf"

# nm -D is only meaningful on ELF platforms
case "$(uname -s 2>/dev/null)" in
	Linux | FreeBSD | NetBSD | OpenBSD | SunOS) ;;
	*)
		printf 'SKIP: nm -D check only runs on ELF platforms\n'
		exit 77
		;;
esac

if ! command -v nm >/dev/null 2>&1; then
	printf 'SKIP: nm not available\n'
	exit 77
fi

LIB="${1}"
if [ -z "$LIB" ]; then
	# Autotools: tests execute from the upnp/ build directory; libtool
	# places the real .so in the .libs/ subdirectory.
	for candidate in .libs/libupnp.so .libs/libupnp.so.[0-9]*; do
		[ -f "$candidate" ] && LIB="$candidate" && break
	done
fi

if [ -z "$LIB" ] || [ ! -f "$LIB" ]; then
	printf 'SKIP: libupnp shared library not found\n'
	exit 77
fi

# nm -D lists only dynamic (exported) symbols.
# T = global text symbol (defined), W = weak global symbol (defined).
# A match means UpnpPrintf is reachable by DSO clients – which is the bug.
if nm -D "$LIB" 2>/dev/null | grep -qE " [TW] ${SYMBOL}$"; then
	printf 'FAIL: %s is exported from %s\n' "$SYMBOL" "$LIB" >&2
	exit 1
fi

printf 'PASS: %s is not in the dynamic symbol table of %s\n' "$SYMBOL" "$LIB"
exit 0
