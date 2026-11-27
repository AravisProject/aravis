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

#### arv_camera_create_stream, arv_device_create_stream and arv_fake_camera_set_fill_pattern

[method@Aravis.Camera.create_stream], [method@Aravis.Device.create_stream] and
[method@Aravis.FakeCamera.set_fill_pattern] take an additional GDestroyNotify
parameter, that will be called when the callback closure data are not useful
anymore, and can be destroyed.

#### Thread safety

Feature accesses are now protected by a recursive lock owned by the Genicam
data. Existing code does not need any change, and keeps working as before.
Code using a device from several threads, for example from a stream
callback, should however be aware of a few points (see
[Thread Safety](thread-safety.html)):

- a single feature access is safe from any thread, but a sequence of feature
  accesses, for example a selector change followed by the access to a
  selected feature, must be protected using [method@Aravis.Camera.lock] and
  [method@Aravis.Camera.unlock], or [method@Aravis.Device.lock_genicam] and
  [method@Aravis.Device.unlock_genicam]. This also applies to the selection
  functions, like [method@Aravis.Camera.select_gain].

- the functions returning a string owned by the Genicam data, like
  [method@Aravis.Camera.get_string] or
  [method@Aravis.Device.get_string_feature_value], are not thread safe. Their
  `dup` counterparts, like [method@Aravis.Camera.dup_string] or
  [method@Aravis.Device.dup_string_feature_value], return a copy instead.

- the lock must not be held while waiting for an internal aravis thread, for
  example during the destruction of a device or a stream.

- the Genicam DOM tree of a device is write locked once the device is
  created: modifications of the tree are refused, unless the write lock is
  first released using [method@Aravis.DomDocument.set_write_lock]. In
  exchange, the DOM read accessors, and the strings and lists they return,
  are thread safe.

#### Event support

Event support is still experimental, and only available when aravis is built
with the `event` meson option. Code using the previous event implementation,
which was only available for GenTL devices, must be adapted (see
[Event Notifications](events.html)):

- the [signal@Aravis.Device::device-event] signal now passes an
  [class@Aravis.Event] instead of the event identifier. A handler with the
  previous signature still compiles, but would read a pointer as an integer.
  The event identifier is returned by [method@Aravis.Event.get_id]:

```C
static void
device_event_cb (ArvDevice *device, ArvEvent *event, gpointer user_data)
{
	printf ("Event 0x%04x\n", arv_event_get_id (event));
}
```

- the signal, as well as the new callback set with
  [method@Aravis.Device.set_event_callback], is emitted from an internal
  thread dedicated to the event delivery. Event features can be read from the
  handler, but feature access sequences shared with other threads must be
  protected using [method@Aravis.Camera.lock].

- the `device_event` virtual function of [class@Aravis.Device] takes an
  [class@Aravis.Event] parameter.
