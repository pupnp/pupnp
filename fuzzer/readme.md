# Fuzzer <!-- omit in toc -->

## 1. Information

- [libFuzzer – a library for coverage-guided fuzz testing](https://llvm.org/docs/LibFuzzer.html)
- [libFuzzer Tutorial](https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md)
- [Fuzzing Forum](https://github.com/google/fuzzing)
- [Reproducing OSS-Fuzz issues](https://google.github.io/oss-fuzz/advanced-topics/reproducing/)

## 2. Other Information

- Fuzzer requires clang; do `export CC=clang` and `export CXX=clang++` before calling `cmake` to compile.
- When debugging, use -O0, otherwise, things get confusing in the debugger.
- Look at `build.sh` to see the actual process.

## 3. Using

```bash
cd fuzzer

# Builds the libraries and all the fuzz targets into fuzzer/build
./build.sh UBSan

# Runs a target (FuzzIxml when none is given), starting from its seed corpus
# in fuzzer/corpus. Runs forever or until a bug is detected.
# If a bug is detected, it generates a testcase file.
./build.sh Run FuzzIxmlOps

# Runs only this particular test case
./build/fuzzer/FuzzIxml fuzzer_testcases/clusterfuzz-testcase-FuzzIxml-5047870085988352
```
