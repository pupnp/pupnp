#! /bin/bash

#
# Assign a default value when testing
CURRENT=${curr_release:-1.14.33}
NEXT=${next_release:-1.14.34}
GITHUB_STEP_SUMMARY="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

sed_script="s/${CURRENT}/${NEXT}/"
echo "${sed_script}" >> "$GITHUB_STEP_SUMMARY"

#
# Simple substitution
sed -i "${sed_script}" docs/changelog_template.txt
sed -i "${sed_script}" docs/Doxyfile
sed -i "${sed_script}" libupnp.spec

#
# Include file two lines before line
line=$(sed -n "/${CURRENT}/=" ChangeLog)
sed -i "$((line-2)) r docs/changelog_template.txt" ChangeLog

#
# Configure.ac is more chalenging
sed_script="/AC_INIT/ ${sed_script}"
sed -i "${sed_script}" configure.ac
