#!/usr/bin/bash
BUILD_DIR="$HOME/devel/pupnp-dev/pupnp"
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
