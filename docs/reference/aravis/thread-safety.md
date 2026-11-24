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
[class@Aravis.GcFeatureNode] availability and access mode functions) are
protected by a recursive lock owned by the Genicam data. A single feature
access is then safe from any thread.

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

The [class@Aravis.Camera] convenience functions using a selector, like
[method@Aravis.Camera.set_trigger] or [method@Aravis.Camera.set_chunks], already
take this lock.

The strings returned by [method@Aravis.Camera.get_string] and
[method@Aravis.Device.get_string_feature_value] are owned by the Genicam data,
and may be modified by a feature access from another thread.
[method@Aravis.Camera.dup_string] and
[method@Aravis.Device.dup_string_feature_value] return a copy instead.

The lock is held during the device accesses, which means a feature access may
have to wait for the completion of a slow device access made from another
thread. It must not be held while waiting for an internal aravis thread, for
example during the destruction of a device or a stream.

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

The [signal@Aravis.Device::device-event] signal and the event callback are
emitted from an internal thread dedicated to the event delivery. See
[Event Notifications](events.html).
