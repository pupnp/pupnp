#! /bin/bash

# If DEBUG is unset, set it to "n"
: <<< "${DEBUG:=n}"

# Assign a default value when testing
: <<< "${GITHUB_STEP_SUMMARY:=/dev/stdout}"

check_release () {
    local TAG_VALUE=$1
    local TAG_NAME=$2

    if [[ -z ${TAG_VALUE} ]]; then
        echo "${TAG_NAME} is empty"
        exit 1
    fi

    local PATTERN="[0-9]+\.[0-9]+\.[0-9]+$"
    if [[ ! ${TAG_VALUE} =~ ${PATTERN} ]]; then
        echo "${TAG_NAME} does not respect the pattern: \"${TAG_VALUE} =~ ${PATTERN}\""
        exit 2
    fi
}

check_release "${next_release:-}" "Next release"

# Creates temporary files
CHANGELOG_TEMPLATE=$(mktemp) || exit 1

# shellcheck disable=SC2329
cleanup_on_exit() {
    rm -f "${CHANGELOG_TEMPLATE}"
}

# Makes sure it will be gone
trap cleanup_on_exit EXIT

################################################################################
# The changelog template
################################################################################
cat > "${CHANGELOG_TEMPLATE}" << EOF
*******************************************************************************
Version ${next_release}
*******************************************************************************



EOF

# Fix the ChangeLog file using the template
cat "${CHANGELOG_TEMPLATE}" ChangeLog > ChangeLog.tmp
mv ChangeLog.tmp ChangeLog

################################################################################
# Fix Doxyfile and spec file using simple sed substitution
################################################################################
sed -i "s/^PROJECT_NUMBER.*/PROJECT_NUMBER         = ${next_release}/" docs/Doxyfile
sed -i "s/^Version:.*/Version: ${next_release}/" libupnp.spec

################################################################################
# Fix the CMakeLists.txt file
#
# We automatically increment the UPNP Patch version for the next development cycle.
################################################################################

# Find out the line number and current patch version of UPNP
upnp_patch_line=$(grep -n "set(UPNP_VERSION_PATCH " CMakeLists.txt | cut -d: -f1)
upnp_patch=$(sed -n "${upnp_patch_line}p" CMakeLists.txt | sed -E 's/.*set\(UPNP_VERSION_PATCH ([0-9]+)\).*/\1/')
new_upnp_patch=$((upnp_patch+1))

# Update the patch version in place
sed -i -E "${upnp_patch_line}s/(set\\(UPNP_VERSION_PATCH )([0-9]+)(\\))/\\1${new_upnp_patch}\\3/" CMakeLists.txt

exit 0
