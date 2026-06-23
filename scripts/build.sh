#! /bin/bash
#
# build.sh — configure, build, and test pupnp locally.
#
# Usage: build.sh [-a] [-d] [-h]
#
#   -a   Enable AddressSanitizer/LeakSanitizer (disables integration tests,
#        which fail when the static lib carries ASan instrumentation)
#   -d   Build source distribution tarball after tests (CPack TBZ2)
#   -h   Show this help

set -e

usage() {
	cat <<EOF
Usage: $0 [-a] [-d] [-h]

  -a   Enable AddressSanitizer/LeakSanitizer
         Disables ixml/upnp integration tests (find_package/add_subdirectory
         builds) that cannot link against an ASan-instrumented static library.
  -d   Build source distribution tarball (cpack -G TBZ2) after tests
  -h   Show this help
EOF
	exit 0
}

asan=0
dist=0

while getopts "adh" opt; do
	case $opt in
		a) asan=1 ;;
		d) dist=1 ;;
		h) usage ;;
		*) usage ;;
	esac
done

cmake_c_flags="-Wall -Wpedantic -O1 -D_FORTIFY_SOURCE=2"
cmake_extra_flags=()

if [ "$asan" -eq 1 ]; then
	cmake_c_flags="-Wall -Wpedantic -fsanitize=address,leak"
	cmake_extra_flags=(
		-DCMAKE_CXX_FLAGS="-fsanitize=address,leak"
		-DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,leak"
		-DIXML_ENABLE_TESTING_INTEGRATION=OFF
		-DUPNP_ENABLE_TESTING_INTEGRATION=OFF
	)
fi

cmake -S . -B build \
	-DBUILD_TESTING=ON \
	-DDOWNLOAD_AND_BUILD_DEPS=ON \
	-DUPNP_ENABLE_OPEN_SSL=ON \
	-DUPNP_ENABLE_UNSPECIFIED_SERVER=ON \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_C_FLAGS="$cmake_c_flags" \
	"${cmake_extra_flags[@]}"

cmake --build build -- -j"$(nproc)"

ctest --test-dir build --output-on-failure --timeout 30 --exclude-regex "^pthreads4w"

cmake -DBUILD_DIR=build -P cmake/post-test.cmake

if [ "$dist" -eq 1 ]; then
	cpack -G TBZ2 --config build/CPackSourceConfig.cmake
fi
