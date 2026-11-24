Title: Event Notifications

# Event Notifications

Event support is experimental, and only available if aravis is built with the
`event` meson option.

Devices can notify the host of events like the end of an exposure or the
start of a frame. Events are enabled with the `EventSelector` and
`EventNotification` features, or with the [method@Aravis.Camera.set_events]
and [method@Aravis.Camera.set_event_notification] convenience functions.

Each notification is delivered as an [class@Aravis.Event], which carries the
event identifier, a timestamp, and the event data. The event data can be read
through the event features of the Genicam description, for example
`EventExposureEndTimestamp`.

## Delivery

Events are delivered by the callback set with
[method@Aravis.Device.set_event_callback], and by the
[signal@Aravis.Device::device-event] signal:

```c
static void
event_cb (void *user_data, ArvEvent *event)
{
	ArvCamera *camera = user_data;

	printf ("Exposure end at %" G_GINT64_FORMAT "\n",
		arv_camera_get_integer (camera, "EventExposureEndTimestamp", NULL));
}

...

arv_camera_set_event_notification (camera, "ExposureEnd", TRUE, &error);
arv_device_set_event_callback (arv_camera_get_device (camera), event_cb, camera, NULL);
```

## Threading

Events are received by an internal thread of the device implementation, which
only acknowledges them and queues them. They are then delivered, one at a
time, by another internal thread dedicated to the event delivery, which
invokes the callback and emits the signal. This keeps the protocol handling
responsive while the callback is running.

Feature accesses are protected by a lock owned by the Genicam data (see
[Thread Safety](thread-safety.html)), which makes the event features safe to
read from the callback, even if the device is also used from another thread.
When the callback, or another thread, uses a selector, the feature access
sequence must be protected by [method@Aravis.Camera.lock] and
[method@Aravis.Camera.unlock]:

```c
static void
event_cb (void *user_data, ArvEvent *event)
{
	ArvCamera *camera = user_data;

	arv_camera_lock (camera);
	arv_camera_set_string (camera, "TimerSelector", "Timer0", NULL);
	printf ("Timer0 duration %g\n", arv_camera_get_float (camera, "TimerDuration", NULL));
	arv_camera_unlock (camera);
}
```

An application wanting to handle events in its main thread can forward them
from the callback, for example using [method@GLib.MainContext.invoke] with a
reference on the [class@Aravis.Event].

The callback must not destroy the device, and
[method@Aravis.Device.set_event_callback] must not be called while holding the
Genicam lock. When [method@Aravis.Device.set_event_callback] returns, the
previous callback is not running anymore.

If the callback is too slow, events are queued up to an internal limit, after
which the oldest events are dropped. The number of dropped events is returned
by [method@Aravis.Device.get_n_dropped_events].

The `arv-camera-test` utility has an `--events` option, which prints the
received events and their associated feature values.
