#!/usr/bin/env python

# Aravis - Digital camera library
#
# Copyright (c) 2011 Emmanuel Pacaud
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307, USA.
#
# Author: Emmanuel Pacaud <emmanuel@gnome.org>

#  If you have installed aravis in a non standard location, you may need
#   to make GI_TYPELIB_PATH point to the correct location. For example:
#
#   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/
#
#  You may also have to give the path to libaravis.so, using LD_PRELOAD or
#  LD_LIBRARY_PATH.

import sys
import time

from gi.repository import Aravis

if len(sys.argv) > 1:
	camera = Aravis.Camera.new (sys.argv[1])
else:
	camera = Aravis.Camera.new (None)

camera.set_region (0,0,128,128)
camera.set_pixel_format (0x01080001) # MONO_8 - GI bug workaround
camera.set_frame_rate (10.0)

[x,y,width,height] = camera.get_region ()

print "Camera vendor :", camera.get_vendor_name ()
print "Camera model  :", camera.get_model_name ()
print "Camera id     :", camera.get_device_id ()
print "ROI           :", width, "x", height, "at", x, ",", y

stream = camera.create_stream (None, None)

for i in range(0,100):
	stream.push_buffer (Aravis.Buffer.new (128*128, None))

print "Start acquisition"

camera.start_acquisition ()

print "Acquisition"

time.sleep (1)

buffer = stream.pop_buffer ()

time.sleep (1)

print "Stop acquisition"

camera.stop_acquisition ()
