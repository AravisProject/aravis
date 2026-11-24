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

Events are received in an internal thread. However, reading an event feature
may require accesses to device registers, and the [class@Aravis.Device]
Genicam data are not thread safe. That's why the callback and the signal are
never invoked from the receiving thread. They are invoked from the thread
default [struct@GLib.MainContext] of the thread which called
[method@Aravis.Device.set_event_callback], or of the thread which created the
device if no callback is set. Event features can then be safely read from
the callback, as long as the device is only used from that thread.

This main context must be iterated. In an application using a
[struct@GLib.MainLoop], nothing more is needed. An acquisition loop running
in a worker thread must use its own main context, and iterate it:

```c
GMainContext *context = g_main_context_new ();

g_main_context_push_thread_default (context);

camera = arv_camera_new (NULL, &error);
arv_device_set_event_callback (arv_camera_get_device (camera), event_cb, camera, NULL);

...

while (running) {
	buffer = arv_stream_timeout_pop_buffer (stream, 100000);

	/* Dispatch the pending events */
	while (g_main_context_iteration (context, FALSE));

	...
}

g_main_context_pop_thread_default (context);
g_main_context_unref (context);
```

Note that a thread which didn't push its own main context uses the global
default one, which means events would be delivered in the main thread.

If the main context is not iterated, events are queued up to an internal
limit, after which the oldest events are dropped. The number of dropped events
is returned by [method@Aravis.Device.get_n_dropped_events].

The `arv-camera-test` utility has an `--events` option, which prints the
received events and their associated feature values.
