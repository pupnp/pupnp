# ./compile.sh proof.cpp -DREAL
BUILD_DIR="$HOME/devel/pupnp-dev/pupnp"
/usr/bin/g++ -std=c++11 -pedantic-errors -Wall $2 -I$BUILD_DIR/googletest-src/googletest/include -I$BUILD_DIR/googletest-src/googlemock/include $1 $BUILD_DIR/lib/libgtestd.a $BUILD_DIR/lib/libgmockd.a -lpthread
