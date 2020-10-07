import gi
import os

gi.require_version ('Aravis', '0.8')

from gi.repository import Aravis,GLib

Aravis.set_fake_camera_genicam_filename (os.getenv ('FAKE_GENICAM_PATH'))

Aravis.enable_interface ("Fake")

try:
    camera = Aravis.Camera.new ("NoCamera")

except GLib.Error as err:
    assert err.matches (Aravis.device_error_quark(), Aravis.DeviceError.NOT_FOUND)

camera = Aravis.Camera.new ("Fake_1")

try:
    camera.get_integer ("Width")
    camera.get_integer ("NoFeature")

    assert False # Not reached

    camera.get_integer ("Height")

except GLib.Error as err:
    assert err.matches (Aravis.device_error_quark(), Aravis.DeviceError.FEATURE_NOT_FOUND)

try:
    camera.get_float ("NoFeature")

    assert False # Not reached

except GLib.Error as err:
    assert err.matches (Aravis.device_error_quark(), Aravis.DeviceError.FEATURE_NOT_FOUND)

try:
    camera.get_string ("NoFeature")

    assert False # Not reached

except GLib.Error as err:
    assert err.matches (Aravis.device_error_quark(), Aravis.DeviceError.FEATURE_NOT_FOUND)

try:
    camera.get_boolean ("NoFeature")

    assert False # Not reached

except GLib.Error as err:
    assert err.matches (Aravis.device_error_quark(), Aravis.DeviceError.FEATURE_NOT_FOUND)

value = camera.get_boolean ("TestBoolean")

assert value is False

camera.set_boolean ("TestBoolean", True)
value = camera.get_boolean ("TestBoolean")

assert value is True
