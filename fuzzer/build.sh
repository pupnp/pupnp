#!/bin/bash -eu

build(){
    export CFLAGS="$1"
    export CXXFLAGS="$1"
    export LIB_FUZZING_ENGINE=-fsanitize=fuzzer

    mkdir build && cd build/
    cmake -DFUZZER=ON -DLIB_FUZZING_ENGINE="$LIB_FUZZING_ENGINE" ../../.
    make -j$(nproc)

    cd fuzzer/

    mkdir FuzzIxml_corpus
    mkdir FuzzIxml_seed_corpus

    cp ../../../ixml/test/testdata/empty_attribute.xml FuzzIxml_seed_corpus/
}

run(){
    DIR=build/fuzzer
    ./$DIR/FuzzIxml   $DIR/FuzzIxml_corpus/   $DIR/FuzzIxml_seed_corpus/
}

if [ $1 == "ASan" ]
then
   build "-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=fuzzer-no-link"
elif [ "$1" == "UBSan" ]
then
   build "-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=array-bounds,bool,builtin,enum,float-divide-by-zero,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unsigned-integer-overflow,unreachable,vla-bound,vptr -fno-sanitize-recover=array-bounds,bool,builtin,enum,float-divide-by-zero,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr -fsanitize=fuzzer-no-link"
elif [ "$1" == "MSan" ]
then
   build "-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=memory -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link"
elif [ "$1" == "Run" ]
then
   run
else
  echo "use: ./$0 ASan | UBSan | MSan | Run"
fi
