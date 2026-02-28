#! /bin/bash

cmake -S . -B build \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	-DBUILD_TESTING=ON \
	-DDOWNLOAD_AND_BUILD_DEPS=ON \
	-DCMAKE_BUILD_TYPE=Debug \
	-DBUILD_TESTING=ON \
	-DCMAKE_C_FLAGS="-Wconversion -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wformat-security -Wmissing-format-attribute -Wpedantic -fsanitize=address,leak" \
	-DCMAKE_CXX_FLAGS="-fsanitize=address,leak" \
	-DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,leak"

cmake --build build -- -j20

cd build || exit 1

ctest --output-on-failure
