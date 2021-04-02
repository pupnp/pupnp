#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
    >&2 echo "Compile with: $0 test_tools.cpp"
    >&2 echo 'Set BUILD_DIR to point to the pupnp build directory'
fi

BUILD_DIR="../.."

TESTNAME=$(/usr/bin/basename -s.cpp "$1")
/usr/bin/g++ -std=c++11 -pedantic-errors -Wall \
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
-lpthread
