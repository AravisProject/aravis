Title: Porting to Aravis 0.10

### Porting to Aravis 0.10

#### Acquisition control

`arv_stream_start_thread()` and `arv_stream_stop_thread()` functions have been
removed in favor of [method@Aravis.Stream.start_acquisition] and
[method@Aravis.Stream.stop_acquisition]. The main difference is acquisition is
not started automatically at stream creation, but should be done afterward. This
maps better with GenAPI, where buffers must be pushed to DataStream objects
before DataStream acquisition is started.
[method@Aravis.Camera.start_acquisition] and
[method@Aravis.Camera.stop_acquisition] do the calls for you, but if you are
using the `ArvDevice` API directly, you have to call the start/stop acquisition
functions.

The optional `delete_buffers` parameter of `arv_stream_stop_thread()` is
replaced by a separate [method@Aravis.Stream.delete_buffers] function.
