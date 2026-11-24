# SPDX-License-Identifier:Unlicense

import gi
import os

gi.require_version ('Aravis', '0.10')

from gi.repository import Aravis

Aravis.set_fake_camera_genicam_filename (os.getenv ('FAKE_GENICAM_PATH'))

Aravis.enable_interface ("Fake")

camera = Aravis.Camera.new ("Fake_1")

assert camera is not None

camera.set_region (10,20,100,200)
camera.set_frame_rate (50.0)
camera.set_pixel_format (Aravis.PIXEL_FORMAT_MONO_8)

(x,y,w,h) = camera.get_region ()

assert x == 10
assert y == 20
assert w == 100
assert h == 200

assert camera.get_pixel_format_as_string () == 'Mono8'

payload = camera.get_payload ()

[x,y,width,height] = camera.get_region ()

callback_map = {
    Aravis.StreamCallbackType.INIT: 0,
    Aravis.StreamCallbackType.START_BUFFER: 0,
    Aravis.StreamCallbackType.BUFFER_DONE: 0,
    Aravis.StreamCallbackType.EXIT: 0,
}


test_user_data = [1, 2, 3]

def callback(user_data, cb_type, buffer):
    assert user_data == test_user_data
    callback_map[cb_type] += 1
    if cb_type == Aravis.StreamCallbackType.BUFFER_DONE:
        assert buffer is not None


stream = camera.create_stream (callback, test_user_data)

for i in range(0,10):
	stream.push_buffer (Aravis.Buffer.new_allocate (payload))

camera.start_acquisition ()

for i in range(0,20):
    buffer = stream.pop_buffer ()
    assert buffer.get_status () == Aravis.BufferStatus.SUCCESS
    if buffer:
       stream.push_buffer (buffer)

camera.stop_acquisition ()

# Explicitly delete the stream here. Otherwise it will likely be garbage collected only
# when the test application exits. This is to test that we receive a StreamCallbackType.EXIT
# event.
del stream

assert callback_map[Aravis.StreamCallbackType.INIT] == 1
assert callback_map[Aravis.StreamCallbackType.START_BUFFER] >= 20
assert callback_map[Aravis.StreamCallbackType.BUFFER_DONE] == callback_map[Aravis.StreamCallbackType.START_BUFFER]
assert callback_map[Aravis.StreamCallbackType.EXIT] == 1

buffer = camera.acquisition (0)
data = buffer.get_data ()

assert len (data) == 20000

# Events, if aravis was built with event support

if hasattr (Aravis.Camera, 'set_events'):
    assert camera.are_events_available ()

    device = camera.get_device ()
    signal_events = []
    callback_events = []

    def device_event_cb (device, event):
        signal_events.append ((event.get_id (), camera.get_integer ("EventExposureEndFrameID")))

    def event_cb (user_data, event):
        assert user_data == test_user_data
        callback_events.append (event.get_id ())

    handler_id = device.connect ("device-event", device_event_cb)
    device.set_event_callback (event_cb, test_user_data)

    camera.set_events ("ExposureEnd")
    assert camera.get_event_notification ("ExposureEnd")

    stream = camera.create_stream (None, None)
    for i in range (0, 5):
        stream.push_buffer (Aravis.Buffer.new_allocate (payload))

    import time

    camera.start_acquisition ()
    for i in range (0, 10):
        buffer = stream.pop_buffer ()
        stream.push_buffer (buffer)
    camera.stop_acquisition ()

    # Events are delivered from an internal thread
    time.sleep (0.1)
    device.disconnect (handler_id)
    device.set_event_callback (None, None)

    assert len (signal_events) > 0
    assert len (callback_events) == len (signal_events)
    for (event_id, frame_id) in signal_events:
        assert event_id == 0x9001
    for i in range (1, len (signal_events)):
        assert signal_events[i][1] != signal_events[i - 1][1]

    camera.set_events (None)

    del stream
