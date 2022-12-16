#!/usr/bin/env python

#  If you have installed aravis in a non standard location, you may need
#   to make GI_TYPELIB_PATH point to the correct location. For example:
#
#   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/
#
#  You may also have to give the path to libaravis.so, using LD_PRELOAD or
#  LD_LIBRARY_PATH.

import threading
import time

import gi
# autopep8: off
gi.require_version('Aravis', '0.8')
from gi.repository import Aravis  # noqa: E402
# autopep8: on

Aravis.enable_interface('Fake')


class UserData:

    def __init__(self) -> None:
        self.stream = None


def callback(user_data, cb_type, buffer):
    print(f'Callback[{threading.get_native_id()}] {cb_type.value_name} {buffer=}')
    if buffer is not None:
        # Fake some light computation here (like copying the buffer). Do not do heavy image processing here
        # since it would block the acquisition thread.
        time.sleep(0.01)
        # Re-enqueue the buffer
        user_data.stream.push_buffer(buffer)


print(f'Main thread[{threading.get_native_id()}]')
cam = Aravis.Camera.new('Fake_1')

user_data = UserData()
stream = cam.create_stream(callback, user_data)
user_data.stream = stream

payload = cam.get_payload()
stream.push_buffer(Aravis.Buffer.new_allocate(payload))

cam.start_acquisition()
time.sleep(1)
cam.stop_acquisition()
