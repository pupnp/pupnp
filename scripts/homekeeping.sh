#! /bin/bash

#
# Assign a default value when testing
CURRENT=${curr_release:-1.14.21}
NEXT=${next_release:-1.14.22}
GITHUB_STEP_SUMMARY="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

script="s/${CURRENT}/${NEXT}/"
#echo "${script}" >> "$GITHUB_STEP_SUMMARY"

#
# Simple substitution
sed -i "${script}" docs/changelog_template.txt
sed -i "${script}" docs/configure_ac_template.txt
sed -i "${script}" docs/Doxyfile
sed -i "${script}" libupnp.spec

#
# Include file two lines before line
line=$(sed -n "/${CURRENT}/=" ChangeLog)
sed -i "$((line-2)) r docs/changelog_template.txt" ChangeLog

#
# Configure.ac is more challenging
script="/AC_INIT/ ${script}"
sed -i "${script}" configure.ac

line=$(sed -n "/AC_INIT/=" configure.ac)
line_ixml=$(sed -n -E "$((line+1)) p" configure.ac)
line_upnp=$(sed -n -E "$((line+2)) p" configure.ac)

search_pattern=".*\[([0-9]+):([0-9]+):([0-9]+)\]\)"

current_ixml=$(echo "${line_ixml}" | sed -E "s/${search_pattern}/\1/")
revision_ixml=$(echo "${line_ixml}" | sed -E "s/${search_pattern}/\2/")
age_ixml=$(echo "${line_ixml}" | sed -E "s/${search_pattern}/\3/")
#revision_ixml=$((revision_ixml+1))

current_upnp=$(echo "${line_upnp}" | sed -E "s/${search_pattern}/\1/")
revision_upnp=$(echo "${line_upnp}" | sed -E "s/${search_pattern}/\2/")
age_upnp=$(echo "${line_upnp}" | sed -E "s/${search_pattern}/\3/")
revision_upnp=$((revision_upnp+1))

sed -i -E \
    "${line} {
        # Print the current line in the pattern
        # space and go to the next one
        n;
        # Copy that line to the hold space
        h;
        # Add the comment on the pattern space
        s/(.*)/dnl #\1/;
        # Exchange pattern and hold spaces
        x;
        # Print, go to the next
        n;
        # Print
        p;
        # Add the comment
        s/(.*)/dnl #\1/;
        # Append to the hold space
        H;
        # Append the hold space to the pattern space
        G;
        # Delete the first line of the pattern space
        D;
}" configure.ac

pattern="(.*,\s\[).*(\]\))"
sed -i -E "$((line+1)) s/$pattern/\1$current_ixml:$revision_ixml:$age_ixml\2/" configure.ac
sed -i -E "$((line+2)) s/$pattern/\1$current_upnp:$revision_upnp:$age_upnp\2/" configure.ac
sed -i -E "$((line+2)) r docs/configure_ac_template.txt" configure.ac

#AC_INIT([libupnp],[1.14.21],[mroberto@users.sourceforge.net])
#AC_SUBST([LT_VERSION_IXML], [12:3:1])
#AC_SUBST([LT_VERSION_UPNP], [19:1:2])
