BUILD_DIR="$HOME/devel/pupnp-dev/pupnp"
/usr/bin/g++ -std=c++11 -pedantic-errors -Wall -o mock_turtle_test.a -I$BUILD_DIR/googletest-src/googletest/include -I$BUILD_DIR/googletest-src/googlemock/include mock_turtle_test.cpp $BUILD_DIR/lib/libgtestd.a $BUILD_DIR/lib/libgmockd.a -lpthread
