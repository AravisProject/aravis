import gi
import os

gi.require_version ('Aravis', '0.8')

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
