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

`./build.sh Run` does not pass the dictionary or the options. To fuzz as
OSS-Fuzz does, add them by hand:

```bash
./build/fuzzer/FuzzDevice -dict=FuzzDevice.dict -max_len=65536 \
	build/FuzzDevice_corpus/ corpus/FuzzDevice/
```

## 4. Targets

| Target           | What it feeds the input to                                                                |
| ---------------- | ----------------------------------------------------------------------------------------- |
| FuzzHttp         | The HTTP parser, as a request or as a response (first byte chooses).                      |
| FuzzIxml         | `ixmlLoadDocument()`, through a temporary file.                                           |
| FuzzIxmlDom      | `ixmlParseBufferEx()`, then a walk over the whole DOM and `ixmlPrintDocument()`.          |
| FuzzIxmlOps      | A sequence of ixml DOM mutation calls, decoded from the input.                            |
| FuzzServiceTable | `getServiceTable()` on a device description document.                                     |
| FuzzSsdp         | SSDP messages: the device M-SEARCH handler and the control point header parsing.          |
| FuzzWebServer    | The miniserver request dispatch, with no device and no virtual directory registered.      |
| FuzzDevice       | The same dispatch, with a device and a virtual directory registered (see below).          |

OSS-Fuzz builds every `Fuzz*` target of this directory. A target can have, beside
its source:

- `<target>.dict`: tokens libFuzzer inserts into the inputs;
- `<target>.options`: libFuzzer options, such as `max_len`;
- `corpus/<target>/`: seed inputs, zipped into `<target>_seed_corpus.zip`.

Seeds that are HTTP requests must keep their CRLF line endings. With
`core.autocrlf` set to `input` or `true`, add them with
`git -c core.autocrlf=false add`, and check with `git ls-files --eol` that
the index shows `i/crlf`.

## 5. FuzzDevice

FuzzWebServer reaches the request dispatch, but with nothing registered, so
SOAP, device-side GENA and the virtual directory answer an error at once.
FuzzDevice covers them, as a peer on the network would reach them.

At start-up (`LLVMFuzzerInitialize()`), it:

1. calls `UpnpInit2()` on the loopback interface, which therefore must have
   an IPv4 address;
2. registers a root device with one service (`/control`, `/event`,
   `/scpd.xml`);
3. registers the virtual directory `/vd`, whose callbacks serve a fixed
   buffer from memory.

Each input is then written to one end of a `socketpair()`, and the other end
is passed to the same dispatch the miniserver uses, as one HTTP request:
SOAP actions and `QueryStateVariable`, GENA SUBSCRIBE, renewal and
UNSUBSCRIBE, and GET, HEAD and POST on the virtual directory and on the
description. Nothing is read from or sent to the network.

The device callback answers with fixed values: the action `Fail` returns
error 401, and every state variable is `42`. It never accepts a
subscription, so no NOTIFY is sent. The subscriptions an input creates are
dropped before the next input.

Limitations:

- The control point side (the responses a pupnp client receives) is not
  covered; see FuzzHttp and FuzzSsdp.
- Sending NOTIFY is not exercised.
- Apart from the subscriptions, the stack state persists between inputs, so
  a bug that needs a particular sequence of requests may not reproduce from a
  single input.
