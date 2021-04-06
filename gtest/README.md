## Compile and run tests

To be able to compile a test you have to add an entry in the local
`./CMakeLists.txt`. You can copy the entry for the `test_template.cpp` and
modify the filenames to your test files. Then you can compile the tests in the
local gtest directory with:

    cmake --build .

Then you can run the tests, for example with:

    ./test_template-static

If the output is messed up with logging information from the library,
redirect stderr to /dev/null:

    ./test_upnpapi-static 2>/dev/null

## Skip tests on Github Workflow Actions

It is possible that you have tests which are failing because fixing the bug it
is showing takes some time. To be able to push bug fixes and create pull
requests you can skip tests if they run on the Github Workflow as Action. The
test checks if the environment variable `GITHUB_ACTIONS` exists and will then
skip the test. An example for this conditional check you can find in the
test_template.cpp. If you want to see what tests are skipped on Github Actions
you can execute the test for example with:

    GITHUB_ACTIONS="true" ./test_template-static

<br />
// Last modified: 2021-04-06
