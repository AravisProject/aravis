Title:Utilities

# Utilities

The main goal of Aravis is to provide a library that interfaces with industrial
cameras. It also provides a set of utilities that help to debug the library,
namely `arv-viewer-0.8`, `arv-tool-0.8`, `arv-camera-test-0.8` and
`arv-fake-gv-camera-0.8`. The version suffix corresponds to the API version, as
several stable series of Aravis can be installed at the same time.

The options for each utility is obtained using `--help` argument.

## ForceIP

An GigE Vision camera can be given a temporary address even if its IP address
doesn't match the interface it's connected to with the `force-ip` command:

```sh
arv-tool-0.10 -n 'Daheng*' force-ip ip=192.168.0.11
```

The selection must match exactly one camera. The mask defaults to
`255.255.255.0` and the gateway to `0.0.0.0`. Use `network` after the camera is
reachable to configure its persistent address.
