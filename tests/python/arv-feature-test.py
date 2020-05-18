#!/usr/bin/env python

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

try:
    camera = Aravis.Camera.new (None)
except TypeError:
    print ("No camera found")
    exit ()

device = camera.get_device ()

device.set_integer_feature_value ("Width", 1024)
device.set_integer_feature_value ("Height", 1024)

print ("Width =  %d" %(device.get_integer_feature_value ("Width")))
print ("Height = %d" %(device.get_integer_feature_value ("Height")))
