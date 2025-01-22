Title: Porting to Aravis 0.10

### Porting to Aravis 0.10

The 0.10 stable series breaks some APIs. Like every stable series, it is
parallel installable with the previous ones.

In most case it is possible to make your code compilable against different
stable versions, by using version checks. For example:

```C
#if ARAVIS_CHECK_VERSION(0,10,0)
    stream = arv_camera_create_stream (test_camera->camera,
                                       stream_calback, &callback_data, NULL,
                                       &error);
#else
    stream = arv_camera_create_stream (test_camera->camera,
                                       stream_calback, &callback_data,
                                       &error);
#endif
```

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

### arv_stream_create_buffers

You should preferably use the new [method@Aravis.Stream.create_buffers]
function, which creates interface native buffers if possible.

#### arv_stream_get_n_owned_buffers

`arv_stream_get_n_buffers` has been renamed
[method@Aravis.Stream.get_n_owned_buffers] to better reflect what it does, and
has a third out parameter returning the number of buffer owned by the stream
receiving thread.

#### arv_camera_create_stream

[method@Aravis.Camera.create_stream] takes an additional GDestroyNotify
parameter, that will be called when the callback closure data are not useful
anymore, and can be destroyed.
