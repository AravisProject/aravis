#!/usr/bin/env python3

# SPDX-License-Identifier:Unlicense

#  If you have installed aravis in a non standard location, you may need
#   to make GI_TYPELIB_PATH point to the correct location. For example:
#
#   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/
#
#  You may also have to give the path to libaravis.so, using LD_PRELOAD or
#  LD_LIBRARY_PATH.

import gi

gi.require_version ('Aravis', '0.8')

from gi.repository import Aravis

Aravis.enable_interface("Fake")
try:
    camera = Aravis.Camera(name="Fake_1")
except TypeError:
    print ("No camera found")
    exit ()

device = camera.get_device ()

device.set_integer_feature_value ("Width", 1024)
device.set_integer_feature_value ("Height", 1024)

print ("Width =  %d" %(device.get_integer_feature_value ("Width")))
print ("Height = %d" %(device.get_integer_feature_value ("Height")))


def pt(x):
    print(type(x), x)

pt(device.get_feature_value("DeviceVersion"))
device.set_feature_value("Width", 640.0)
pt(device.get_feature_value("Width"))
device.set_feature_value("AcquisitionFrameRate", 2)
pt(device.get_feature_value("AcquisitionFrameRate"))
device.set_feature_value("TestBoolean", True)
pt(device.get_feature_value("TestBoolean"))
try:
    device.set_feature_value("TestBoolean", None)
except TypeError:
    pass
else:
    print("Expected TypeError")
device.set_feature_value("TestBoolean", 0)
pt(device.get_feature_value("TestBoolean"))
device.set_feature_value("TestStringReg", "TestValue")
pt(device.get_feature_value("TestStringReg"))
pt(device.get_feature_value("GainAuto"))
