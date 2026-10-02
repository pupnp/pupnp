#!/bin/bash -eu

compile() {
   cmake --build "${BUILD_DIR}"
}

build() {
   export CFLAGS="$1"
   export CXXFLAGS="$1"
   export LIB_FUZZING_ENGINE=-fsanitize=fuzzer

   # echo CC="${CC}"
   # echo CXX="${CXX}"
   # echo CFLAGS="${CFLAGS}"
   # echo CXXFLAGS="${CXXFLAGS}"

   # The same build OSS-Fuzz does: the whole tree, from the top level.
   rm -rf "${BUILD_DIR}"
   cmake --fresh -DFUZZER=ON -DLIB_FUZZING_ENGINE="$LIB_FUZZING_ENGINE" -S .. -B "${BUILD_DIR}" &&
      cmake --build "${BUILD_DIR}"
}

# New inputs go to build/<target>_corpus, the seeds come from corpus/<target>.
run() {
   mkdir -p "${BUILD_DIR}/$1_corpus"
   ./"${BUILD_DIR}"/fuzzer/"$1" "${BUILD_DIR}/$1_corpus/" corpus/"$1"/
}

usage() {
   echo "usage: $0 ASan | UBSan | MSan | Run [target] | compile"
}

if [ $# -eq 0 ]; then
   echo "Error: No arguments supplied"
   usage
   exit 1
fi

export CC=clang
export CXX=clang++
export BUILD_DIR=build

if [ "$1" == "ASan" ]; then
   build "-g -O0 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=fuzzer-no-link"
elif [ "$1" == "UBSan" ]; then
   build "-g -O0 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION \
   -fsanitize=address,array-bounds,bool,builtin,enum,float-divide-by-zero,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unsigned-integer-overflow,unreachable,vla-bound,vptr \
   -fno-sanitize-recover=array-bounds,bool,builtin,enum,float-divide-by-zero,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr -fsanitize=fuzzer-no-link"
elif [ "$1" == "MSan" ]; then
   build "-g -O0 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=memory -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link"
elif [ "$1" == "Run" ]; then
   run "${2:-FuzzIxml}"
elif [ "$1" == "compile" ]; then
   compile
else
   echo "Error: Wrong arguments supplied"
   usage
fi
