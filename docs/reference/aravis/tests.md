Title: Unit Tests

# Unit Tests

Aravis has a set of unit tests that helps to catch regressions and memory leaks
during the development. The test suite is run using the following commands:

```sh
ninja test
```

The is a small helper script that run the same tests under valgrind memmory
checker

```sh
../tests/valgrind-memcheck
```

All the code is not covered yet by the tests. Code coverage can be obtained
using:

```sh
meson configure -Db_coverage=true
ninja coverage
```

The report is published in `build/meson-logs/coveragereport/index.html`. Help on
code coverage improvement is welcome.
