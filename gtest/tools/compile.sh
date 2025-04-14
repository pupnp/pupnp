#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
    >&2 echo "Compile with: $0 test_tools.cpp"
    >&2 echo 'Set BUILD_DIR to point to the pupnp build directory'
fi

BUILD_DIR="../../build"

TESTNAME=$(/usr/bin/basename -s.cpp "$1")

compile_flags=(\
-std=c++14 \
-pedantic-errors \
-Wall \
-o"$TESTNAME".a \
-I"$BUILD_DIR"/_deps/googletest-src/googletest/include \
-I"$BUILD_DIR"/_deps/googletest-src/googlemock/include \
-I"$BUILD_DIR" \
-I"$BUILD_DIR"/upnp/src \
-I"$BUILD_DIR"/upnp/inc \
-I"$BUILD_DIR"/upnp/src/inc \
-I"$BUILD_DIR"/upnp/src/threadutil \
-I"$BUILD_DIR"/ixml/inc \
-DUPNP_ENABLE_IPV6 \
"$1" \
"$BUILD_DIR"/lib/libgtestd.a \
"$BUILD_DIR"/lib/libgmockd.a \
"$BUILD_DIR"/upnp/libupnp.a \
"$BUILD_DIR"/ixml/libixml.a \
-lpthread \
)

#
# compile_flags.txt did not work for clangd, I'll leave it commented
#
if false; then
    truncate -s 0 compile_flags.txt
    for i in "${!compile_flags[@]}"; do
        echo "${compile_flags[$i]}" >> compile_flags.txt
    done
    echo >> compile_flags.txt
fi

/usr/bin/g++ "${compile_flags[@]}"

