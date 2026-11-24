Title: Thread Safety

# Thread Safety

Aravis objects are generally not thread safe, which means one can not use the
same object simultaneously from different threads without using mutexes. But
it is perfectly fine to use different aravis objects in different threads,
with the exception of the [class@Aravis.Interface] instances.

## Feature access

The feature accesses of [class@Aravis.Camera], [class@Aravis.Device],
[class@Aravis.ChunkParser], and of the Genicam nodes
([iface@Aravis.GcInteger], [iface@Aravis.GcFloat], [iface@Aravis.GcString],
[class@Aravis.GcEnumeration], [class@Aravis.GcBoolean],
[class@Aravis.GcCommand], [iface@Aravis.GcRegister], and the
[class@Aravis.GcFeatureNode] availability and value functions) are protected
by a recursive lock owned by the Genicam data. A single feature access is
then safe from any thread.

A sequence of feature accesses is not atomic though. When the device is used
from several threads, a selector change followed by the access to a selected
feature must be protected using [method@Aravis.Camera.lock] and
[method@Aravis.Camera.unlock], or [method@Aravis.Device.lock_genicam] and
[method@Aravis.Device.unlock_genicam]:

```c
arv_camera_lock (camera);
arv_camera_set_string (camera, "TriggerSelector", "FrameStart", NULL);
arv_camera_set_string (camera, "TriggerMode", "On", NULL);
arv_camera_unlock (camera);
```

The same applies to the selection functions, like
[method@Aravis.Camera.select_gain] or
[method@Aravis.Camera.gv_select_stream_channel]. The [class@Aravis.Camera]
convenience functions using a selector internally, like
[method@Aravis.Camera.set_trigger] or [method@Aravis.Camera.set_chunks], or
accessing a set of related features, like [method@Aravis.Camera.get_region],
already take the lock.

The lock is held during the device accesses, which means a feature access may
have to wait for the completion of a slow device access made from another
thread. It must not be held while waiting for an internal aravis thread, for
example during the destruction of a device or a stream.

## Strings

Some functions return a string owned by the Genicam data, which may be
changed, or even freed, by a feature access from another thread. They are not
thread safe, and have a `dup` counterpart returning a copy:

| Not thread safe                                  | Thread safe                                      |
|--------------------------------------------------|--------------------------------------------------|
| [method@Aravis.Camera.get_string]                | [method@Aravis.Camera.dup_string]                |
| [method@Aravis.Camera.get_vendor_name]           | [method@Aravis.Camera.dup_vendor_name]           |
| [method@Aravis.Camera.get_model_name]            | [method@Aravis.Camera.dup_model_name]            |
| [method@Aravis.Camera.get_device_serial_number]  | [method@Aravis.Camera.dup_device_serial_number]  |
| [method@Aravis.Camera.get_device_id]             | [method@Aravis.Camera.dup_device_id]             |
| [method@Aravis.Device.get_string_feature_value]  | [method@Aravis.Device.dup_string_feature_value]  |
| [method@Aravis.ChunkParser.get_string_value]     | [method@Aravis.ChunkParser.dup_string_value]     |
| [method@Aravis.GcString.get_value]               | [method@Aravis.GcString.dup_value]               |
| [method@Aravis.GcFeatureNode.get_value_as_string]| [method@Aravis.GcFeatureNode.dup_value_as_string]|

Language bindings copy the returned strings after the function call, which
leaves a window during which the string may be changed by another thread. They
should also use the `dup` functions.

Enumeration entry names, as returned by
[method@Aravis.GcEnumeration.get_string_value] or the `dup_available`
functions, and the feature descriptions, are not changed after the Genicam
data loading, and are safe to use.

## Low level API

The [class@Aravis.GcPropertyNode] functions, the [class@Aravis.Evaluator] and
the [class@Aravis.DomNode] tree API, as well as [method@Aravis.Gc.set_buffer],
are not protected. When the Genicam data are used from several threads, they
must be called with the lock held, using [method@Aravis.Gc.lock].

## Signals and callbacks

A possible trap is that glib signal callbacks are called from the thread that
emitted the corresponding signal. For example, the
[signal@Aravis.Stream::new-buffer] callback is emitted from the stream packet
receiving thread, which is an internal thread created by
[class@Aravis.GvStream]. Feature accesses are safe from this callback, but
most of the [class@Aravis.Stream] functions are not, the exceptions being
[method@Aravis.Stream.push_buffer], [method@Aravis.Stream.pop_buffer],
[method@Aravis.Stream.try_pop_buffer] and
[method@Aravis.Stream.timeout_pop_buffer].
