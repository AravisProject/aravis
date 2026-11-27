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

The GigE Vision tests (`fakegv`) run against `ArvGvFakeCamera` on
127.0.0.1:3956, once per stream receive path: `fakegv` with the defaults,
`fakegv-rx-batch-0`, `fakegv-no-fast-fail` and `fakegv-no-connect` with
`ARV_GV_STREAM_RX_BATCH=0`, `ARV_GV_STREAM_FAST_FAIL=0` and
`ARV_GV_STREAM_CONNECT=0` (see the Ethernet devices page). They never run in
parallel, and they fail if another fake camera holds that port.

On Linux the receive path depends on privileges: with `CAP_NET_RAW` (root) the
stream uses the packet socket loop, otherwise the standard socket loop; there
`/fakegv/short-datagrams-standard-socket` also runs the short datagram case on
the standard loop. Fast fail and the connected socket are off by default outside
Apple, so `fakegv-no-fast-fail` and `fakegv-no-connect` repeat the defaults
there, `/fakegv/repeated-frame-id` is skipped (it is about fast fail) and
`/fakegv/rx-errors` is skipped (`recvmsg_x` is Apple only).

`arv-gv-stream-bench` measures the stream receiver on the loopback interface
against an `arv-fake-gv-camera-0.10` it launches from the build tree, and prints
one tab separated row of frame counts, stream infos, sender counters and CPU
time. It is not part of the default test setup; a short run is
`meson test --suite bench`, and `--help` lists its options, for example:

```sh
tests/arv-gv-stream-bench --duration 10 --frame-rate 200 --loss 0.001 --buffers 16
```

All the code is not covered yet by the tests. Code coverage can be obtained
using:

```sh
meson configure -Db_coverage=true
ninja coverage
```

The report is published in `build/meson-logs/coveragereport/index.html`. Help on
code coverage improvement is welcome.
