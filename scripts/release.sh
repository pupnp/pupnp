#!/bin/bash
#
# release.sh — create and push a release tag to trigger the GitHub Actions release workflow.
#
# Purpose:
#   Validates that the version in CMakeLists.txt represents a genuine step
#   forward before committing to a release.  Pushing the tag is the point of
#   no return — it immediately triggers the release workflow on GitHub Actions,
#   which builds the tarball, creates a draft GitHub release, and opens a
#   home-keeping PR.  This script makes that action deliberate and safe.
#
# Usage:
#   Run from the repository root:
#     ./scripts/release.sh       # full validation + confirmation + push
#     ./scripts/release.sh -n    # print the tag that would be pushed and exit
#
# What it does:
#   1. Derives the current PUPNP release version from CMakeLists.txt and
#      prints the tag name (visible even if a later check aborts).
#   2. Verifies that the current branch is main.
#   3. Verifies that the working tree is clean (no uncommitted changes).
#   4. Finds the most recent release tag on the repository.
#   5. Aborts if the current version is not strictly greater than the last
#      release (same version = forgot to bump; older version = something is wrong).
#   6. Aborts if the tag already exists locally or on the remote (double-release guard).
#   7. In dry-run mode (-n), exits here — all checks passed, nothing pushed.
#   8. Shows a summary and asks for explicit confirmation before pushing.
#   9. Creates and pushes the tag, which starts the release workflow.
#
# After the tag is pushed, monitor the workflow with:
#   gh run watch --exit-status
#
set -e

# ------------------------------------------------------------------------------
# Argument parsing.
# ------------------------------------------------------------------------------
DRY_RUN=0
if [[ "${1:-}" == "-n" ]]; then
    DRY_RUN=1
fi

# ------------------------------------------------------------------------------
# Derive the current PUPNP umbrella version from CMakeLists.txt.
# The umbrella version is the sum of the IXML and UPNP component versions,
# mirroring the math CMake performs at configure time.
# Done first so the tag name is always visible, even if a later check aborts.
# ------------------------------------------------------------------------------
ixml_major=$(grep -E "^set\(IXML_VERSION_MAJOR" CMakeLists.txt | grep -oE '[0-9]+')
ixml_minor=$(grep -E "^set\(IXML_VERSION_MINOR" CMakeLists.txt | grep -oE '[0-9]+')
ixml_patch=$(grep -E "^set\(IXML_VERSION_PATCH" CMakeLists.txt | grep -oE '[0-9]+')
upnp_major=$(grep -E "^set\(UPNP_VERSION_MAJOR" CMakeLists.txt | grep -oE '[0-9]+')
upnp_minor=$(grep -E "^set\(UPNP_VERSION_MINOR" CMakeLists.txt | grep -oE '[0-9]+')
upnp_patch=$(grep -E "^set\(UPNP_VERSION_PATCH" CMakeLists.txt | grep -oE '[0-9]+')
pupnp_major=$((ixml_major + upnp_major))
pupnp_minor=$((ixml_minor + upnp_minor))
pupnp_patch=$((ixml_patch + upnp_patch))

CURRENT_VERSION="${pupnp_major}.${pupnp_minor}.${pupnp_patch}"
CURRENT_TAG="release-${CURRENT_VERSION}"

echo "Current tag to push is ${CURRENT_TAG}"

# ------------------------------------------------------------------------------
# Sanity checks — catch obvious mistakes before doing any real work.
# ------------------------------------------------------------------------------

# Releases must be cut from main to keep the tag history linear and ensure
# the home-keeping branch the workflow creates can be cleanly merged back.
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [[ "$CURRENT_BRANCH" != "main" ]]; then
    echo "ERROR: You are on branch '${CURRENT_BRANCH}', not 'main'."
    echo "       Switch to main before releasing."
    exit 1
fi

# An unclean tree means uncommitted changes would be missing from the tag.
# 'git status --porcelain' produces no output when the tree is clean.
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Working tree has uncommitted changes:"
    git status --short
    echo ""
    echo "       Commit or stash them before releasing."
    exit 1
fi

# ------------------------------------------------------------------------------
# Find the most recent release tag.
# Tags follow the "release-X.Y.Z" convention; sort -V orders them correctly.
# ------------------------------------------------------------------------------
LAST_TAG=$(git tag -l 'release-*' | sort -V | tail -1)

if [[ -z "$LAST_TAG" ]]; then
    echo "No previous release tag found — this will be the first release."
    LAST_VERSION="(none)"
else
    LAST_VERSION="${LAST_TAG#release-}"

    # Use sort -V to determine which version is newer.  If CURRENT_VERSION is
    # not the greatest of the two (or they are equal), the version was not bumped.
    NEWER=$(printf '%s\n%s\n' "$LAST_VERSION" "$CURRENT_VERSION" | sort -V | tail -1)

    if [[ "$CURRENT_VERSION" == "$LAST_VERSION" ]]; then
        echo "ERROR: Current version ${CURRENT_VERSION} is the same as the last release (${LAST_TAG})."
        echo "       Bump the version in CMakeLists.txt before releasing."
        exit 1
    fi

    if [[ "$NEWER" != "$CURRENT_VERSION" ]]; then
        echo "ERROR: Current version ${CURRENT_VERSION} is older than the last release (${LAST_TAG})."
        echo "       Check the version numbers in CMakeLists.txt."
        exit 1
    fi
fi

# ------------------------------------------------------------------------------
# Guard against a duplicate tag — creating it twice would fail on push anyway,
# but catching it here gives a clearer error message.
# ------------------------------------------------------------------------------
if git tag -l "$CURRENT_TAG" | grep -q "$CURRENT_TAG"; then
    echo "ERROR: Tag ${CURRENT_TAG} already exists locally."
    exit 1
fi

if git ls-remote --tags origin "refs/tags/${CURRENT_TAG}" | grep -q "${CURRENT_TAG}"; then
    echo "ERROR: Tag ${CURRENT_TAG} already exists on the remote."
    exit 1
fi

# ------------------------------------------------------------------------------
# In dry-run mode all checks have now passed — exit without pushing.
# ------------------------------------------------------------------------------
if [[ "$DRY_RUN" -eq 1 ]]; then
    exit 0
fi

# ------------------------------------------------------------------------------
# Show a summary and ask for explicit confirmation before doing anything
# irreversible.  Pushing the tag immediately triggers the release workflow.
# ------------------------------------------------------------------------------
echo ""
echo "  Last release : ${LAST_TAG:-none}"
echo "  New tag      : ${CURRENT_TAG}"
echo ""
read -r -p "Push '${CURRENT_TAG}' and start the release workflow? [y/N] " answer
if [[ "$answer" != "y" && "$answer" != "Y" ]]; then
    echo "Aborted."
    exit 0
fi

# ------------------------------------------------------------------------------
# Create the tag locally and push it.  The push triggers the GitHub Actions
# release workflow defined in .github/workflows/release.yml.
# ------------------------------------------------------------------------------
git tag "$CURRENT_TAG"
git push origin "$CURRENT_TAG"

echo ""
echo "Tag ${CURRENT_TAG} pushed."
echo "Monitor the release workflow with:"
echo "  gh run watch --exit-status"
