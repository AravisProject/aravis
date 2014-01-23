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
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
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

Aravis.enable_interface ("Fake")

try:
	if len(sys.argv) > 1:
		camera = Aravis.Camera.new (sys.argv[1])
	else:
		camera = Aravis.Camera.new (None)
except:
	print ("No camera found")
	exit ()

camera.set_region (0,0,128,128)
camera.set_frame_rate (10.0)
camera.set_pixel_format (Aravis.PIXEL_FORMAT_MONO_8)

payload = camera.get_payload ()

[x,y,width,height] = camera.get_region ()

print "Camera vendor : %s" %(camera.get_vendor_name ())
print "Camera model  : %s" %(camera.get_model_name ())
print "Camera id     : %s" %(camera.get_device_id ())
print "ROI           : %dx%d at %d,%d" %(width, height, x, y)
print "Payload       : %d" %(payload)
print "Pixel format  : %s" %(camera.get_pixel_format_as_string ())

stream = camera.create_stream (None, None)

for i in range(0,10):
	stream.push_buffer (Aravis.Buffer.new_allocate (payload))

print "Start acquisition"

camera.start_acquisition ()

print "Acquisition"

for i in range(0,20):
	buffer = stream.pop_buffer ()
	print buffer
	if buffer:
		stream.push_buffer (buffer)

print "Stop acquisition"

camera.stop_acquisition ()

