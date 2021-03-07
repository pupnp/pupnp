BUILD_DIR="$HOME/devel/pupnp-dev/pupnp"
/usr/bin/gcc -pedantic-errors -Wall -DEXECUTABLE -o mylib.a mylib.c
/usr/bin/gcc -pedantic-errors -Wall -c mylib.c
/usr/bin/g++ -std=c++11 -pedantic-errors -Wall -o test_mylib.a -I"$BUILD_DIR"/googletest-src/googletest/include -I"$BUILD_DIR"/googletest-src/googlemock/include test_mylib.cpp "$BUILD_DIR"/lib/libgtestd.a "$BUILD_DIR"/lib/libgmockd.a ./mylib.o -lpthread
