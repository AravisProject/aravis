# SPDX-License-Identifier:Unlicense

import gi
import time
import os

gi.require_version ('Aravis', '0.10')
gi.require_version("Gst", "1.0")

from gi.repository import Aravis,GLib
from gi.repository import Gst

Aravis.set_fake_camera_genicam_filename (os.getenv ('FAKE_GENICAM_PATH'))

Aravis.enable_interface ("Fake")

Gst.init(None)

pipeline = Gst.parse_launch("aravissrc name=src camera-name=Fake_1 ! fakesink")
assert pipeline != None

pipeline.set_state(Gst.State.PLAYING)

time.sleep(1)

src = pipeline.get_by_name("src")

src.emit("software-trigger")

success = src.emit("set-float-feature", "ExposureTimeAbs", 5000.0)   # whatever your signal returns
assert success

success = src.emit("set-boolean-feature", "NonExistentFeature", True)   # whatever your signal returns
assert not success

pipeline.set_state(Gst.State.NULL)
