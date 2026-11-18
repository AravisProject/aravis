Title: Unit Tests

# Unit Tests

Aravis has a set of unit tests that helps to catch regressions and memory leaks
during the development. The test suite is run using the following commands:

```sh
meson test
```

There is a meson test setup for running tests under valgrind memmory checker:

```sh
meson test --setup=valgrind
```

All the code is not covered yet by the tests. Code coverage can be obtained
using:

```sh
meson configure -Db_coverage=true
ninja coverage
```

The report is published in `build/meson-logs/coveragereport/index.html`. Help on
code coverage improvement is welcome.
