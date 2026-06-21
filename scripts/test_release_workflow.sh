#!/bin/bash
#
# test_release_workflow.sh — local simulation of the GitHub Actions release workflow.
#
# Purpose:
#   Validates that the full release process works end-to-end without triggering
#   a real GitHub release.  Run this after changes to any of the following to
#   catch regressions before pushing:
#     - scripts/home_keeping.sh
#     - scripts/parse_release.sh
#     - scripts/find_out_bz2_name.sh
#     - CMakeLists.txt CPack configuration
#
# Usage:
#   Run from the repository root:
#     ./scripts/test_release_workflow.sh
#
# What it does:
#   1. Derives the current release tag from CMakeLists.txt version numbers.
#   2. Creates a temporary git branch to isolate file modifications.
#   3. Runs each step of the release workflow in order.
#   4. Shows the resulting CMakeLists.txt diff (the home-keeping version bump).
#   5. Cleans up the branch and all generated artifacts on exit (even on error).
#
# The GitHub Actions environment variables (GITHUB_ENV, GITHUB_OUTPUT,
# GITHUB_STEP_SUMMARY) are redirected to temporary files / stdout so the
# supporting scripts work unchanged outside of CI.
#
set -e

echo "=========================================="
echo "    Testing Release Process Locally       "
echo "=========================================="

# Derive the current PUPNP umbrella version from CMakeLists.txt.
# CMake computes it as IXML + UPNP for each component; we mirror that math here
# so the script stays correct as the version advances without manual updates.
ixml_major=$(grep -E "^set\(IXML_VERSION_MAJOR" CMakeLists.txt | grep -oE '[0-9]+')
ixml_minor=$(grep -E "^set\(IXML_VERSION_MINOR" CMakeLists.txt | grep -oE '[0-9]+')
ixml_patch=$(grep -E "^set\(IXML_VERSION_PATCH" CMakeLists.txt | grep -oE '[0-9]+')
upnp_major=$(grep -E "^set\(UPNP_VERSION_MAJOR" CMakeLists.txt | grep -oE '[0-9]+')
upnp_minor=$(grep -E "^set\(UPNP_VERSION_MINOR" CMakeLists.txt | grep -oE '[0-9]+')
upnp_patch=$(grep -E "^set\(UPNP_VERSION_PATCH" CMakeLists.txt | grep -oE '[0-9]+')
pupnp_major=$((ixml_major + upnp_major))
pupnp_minor=$((ixml_minor + upnp_minor))
pupnp_patch=$((ixml_patch + upnp_patch))
CURRENT_TAG="release-${pupnp_major}.${pupnp_minor}.${pupnp_patch}"

echo "Derived tag from CMakeLists.txt: ${CURRENT_TAG}"

# Redirect GitHub Actions special files to local equivalents.
# GITHUB_ENV:    key=value pairs shared between workflow steps (via source).
# GITHUB_OUTPUT: step output parameters (written but not consumed locally).
# GITHUB_STEP_SUMMARY: markdown summary printed to stdout here.
export GITHUB_STEP_SUMMARY=/dev/stdout
export GITHUB_ENV=$(mktemp)
export GITHUB_OUTPUT=$(mktemp)
export DEBUG="y"

ORIGINAL_BRANCH=$(git rev-parse --abbrev-ref HEAD)
# Timestamped name avoids collisions if a previous run was interrupted.
TEST_BRANCH="test-release-$(date +%s)"
# Initialised empty; set after step 4 so cleanup can remove the right file.
BZ2_NAME=""

# Cleanup runs on EXIT (covers both success and any set -e abort).
cleanup() {
    # Undo home_keeping.sh file modifications before switching branches,
    # otherwise they bleed into the working tree of the original branch.
    # Restore only the known files home_keeping.sh touches; a blanket
    # 'git restore .' would also undo any uncommitted changes to other files.
    git restore CMakeLists.txt ChangeLog docs/Doxyfile libupnp.spec 2>/dev/null || true
    git checkout "$ORIGINAL_BRANCH" 2>/dev/null || true
    git branch -D "$TEST_BRANCH" 2>/dev/null || true
    [[ -n "$BZ2_NAME" ]] && rm -f "$BZ2_NAME" "${BZ2_NAME}.sha256"
    rm -rf _CPack_Packages/
    rm -f "$GITHUB_ENV" "$GITHUB_OUTPUT"
}
trap cleanup EXIT

# Isolate home_keeping.sh file modifications from the working branch.
git checkout -b "$TEST_BRANCH"

echo "[1/6] Parsing tag '${CURRENT_TAG}'..."
# Validates the tag format, extracts curr_release and next_release, and writes
# them to $GITHUB_ENV so subsequent steps can source them.
scripts/parse_release.sh "${CURRENT_TAG}"
# set -a makes every assignment in the sourced file automatically exported,
# mirroring how GitHub Actions shares GITHUB_ENV values between steps.
set -a; source "$GITHUB_ENV"; set +a
echo "Next release parsed as: $next_release"

echo -e "\n[2/6] Configuring with CMake..."
# Generates build/CPackSourceConfig.cmake needed by step 3.
cmake -B build

echo -e "\n[3/6] Generating Tarball with CPack..."
# Produces libupnp-X.Y.Z.tar.bz2 — the source distribution (replaces 'make distcheck').
cpack -G TBZ2 --config build/CPackSourceConfig.cmake

echo -e "\n[4/6] Finding out bz2 name..."
# Locates the generated tarball by glob and writes bz2_name to $GITHUB_ENV.
scripts/find_out_bz2_name.sh
set -a; source "$GITHUB_ENV"; set +a
BZ2_NAME="$bz2_name"
echo "Generated Tarball name: $BZ2_NAME"

echo -e "\n[5/6] Generating SHA256 checksum..."
sha256sum "$BZ2_NAME" > "${BZ2_NAME}.sha256"
cat "${BZ2_NAME}.sha256"

echo -e "\n[6/6] Running Home Keeping script..."
# Updates ChangeLog, docs/Doxyfile, libupnp.spec, and bumps UPNP_VERSION_PATCH
# in CMakeLists.txt to prepare the tree for the next development cycle.
scripts/home_keeping.sh

echo -e "\n=========================================="
echo "          Release Process Success!        "
echo "=========================================="
echo "Changes to CMakeLists.txt:"
git diff CMakeLists.txt
