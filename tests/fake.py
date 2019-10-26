import gi

gi.require_version ('Aravis', '0.8')

from gi.repository import Aravis

Aravis.enable_interface ("Fake")

camera = Aravis.Camera.new ("Fake_1")

assert camera != None

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

stream = camera.create_stream (None, None)

for i in range(0,10):
	stream.push_buffer (Aravis.Buffer.new_allocate (payload))

camera.start_acquisition ()

for i in range(0,20):
    buffer = stream.pop_buffer ()
    assert buffer.get_status () == Aravis.BufferStatus.SUCCESS
    if buffer:
       stream.push_buffer (buffer)

camera.stop_acquisition ()
