BUILD_DIR="$HOME/devel/pupnp-dev/pupnp"
g++ -std=c++11 -pedantic-errors -Wall -o test_tools.a -I"$BUILD_DIR"/googletest-src/googletest/include -I"$BUILD_DIR"/googletest-src/googlemock/include test_tools.cpp "$BUILD_DIR"/lib/libgtestd.a "$BUILD_DIR"/lib/libgmockd.a -lpthread
