#!/usr/bin/env python

import sys
import gi
import signal

gi.require_version('Aravis', '0.8')

from gi.repository import Aravis,GLib

class SIGINT_handler():
    def __init__(self):
        self.SIGINT = False

    def signal_handler(self, signal, frame):
        print('You pressed Ctrl+C!')
        self.SIGINT = True

handler = SIGINT_handler()
signal.signal(signal.SIGINT, handler.signal_handler)

try:
    if len(sys.argv) > 1:
        print ("Looking for camera '" + sys.argv[1] + "'");
        camera = Aravis.Camera.new (sys.argv[1])
    else:
        print ("Looking for the first available camera");
        camera = Aravis.Camera.new (None)
except GLib.Error as err:
    print ("No camera found: " + err.message)
    exit ()

camera.set_region(0, 0, 100, 100)
camera.set_frame_rate(20.0)
region = camera.get_region()

print("vendor name         = " + camera.get_vendor_name())
print("model name          = " + camera.get_model_name())
print("region              = {0}".format(region))

stream = camera.create_stream()

payload = camera.get_payload()
for i in range(10):
    stream.push_buffer(Aravis.Buffer.new(payload))

camera.start_acquisition()

counter=0

while not handler.SIGINT:
    buffer = stream.timeout_pop_buffer(1000000)
    if buffer != None:
        print("Buffer {0}x{0} {1}".format(buffer.get_image_width(), buffer))
        stream.push_buffer(buffer)

    if counter % 10 == 0:
        width = ((counter / 10) % 2 + 1) * 100
        print (width)
        camera.stop_acquisition()
        stream.stop_thread(True)

        camera.set_region(0, 0, width, width)
        payload = camera.get_payload()

        for i in range(10):
            stream.push_buffer(Aravis.Buffer.new(payload))

        stream.start_thread()
        camera.start_acquisition()

    counter = counter + 1

camera.start_acquisition()
