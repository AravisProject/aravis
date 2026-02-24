# SPDX-License-Identifier:Unlicense

import gi
import os

gi.require_version ('Aravis', '0.10')

from gi.repository import Aravis,GLib

Aravis.set_fake_camera_genicam_filename (os.getenv ('FAKE_GENICAM_PATH'))

Aravis.enable_interface ("Fake")

camera = Aravis.Camera.new ("Fake_1")
device = camera.props.device

device.write_memory (0x48, b'0123\x00')
bytes = device.read_memory(0x48, 5)

assert bytes == b'0123\x00'

try:
    bytes = device.write_memory (0x10000, b'test')

except GLib.Error as err:
    assert err.matches (Aravis.device_error_quark(), Aravis.DeviceError.INVALID_PARAMETER)

device.write_register (0x48, 123)
value = device.read_register (0x48)

assert value == 123

genicam = device.get_genicam_xml ()

assert genicam != None
