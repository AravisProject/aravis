#!/usr/bin/env python

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

# Hexadecimal dump code from http://www.alexonlinux.com/hex-dump-functions

def DumpBuffer (buf, length, caption="", dest=sys.stdout):
	def GetPrintableChar(str):
		if str.isalpha():
			return str
		else:
			return '.'

	dest.write('---------> %s <--------- (%d bytes)\n' % (caption, length))
	dest.write('       +0          +4          +8          +c           0   4   8   c\n')
	i = 0
	while i < length:
		if length - i > 16:
			l = 16
		else:
			l = length - i

		dest.write('+%04x  ' % i)
		s = ' '.join(["%02x" % ord(c) for c in buf[i:i + l]])
		dest.write(s)
		sp = 49 - len(s)
		dest.write(' ' * sp)
		s = ''.join(["%c" % GetPrintableChar(c) for c in buf[i:i + l]])
		dest.write(s)
		dest.write('\n')

		i = i + 16

Aravis.enable_interface ("Fake")

try:
	if len(sys.argv) > 1:
		camera = Aravis.Camera.new (sys.argv[1])
	else:
		camera = Aravis.Camera.new (None)
except:
	print ("No camera found")
	exit ()

payload = camera.get_payload ()

[x,y,width,height] = camera.get_region ()

print "Camera vendor : %s" %(camera.get_vendor_name ())
print "Camera model  : %s" %(camera.get_model_name ())
print "Camera id     : %s" %(camera.get_device_id ())
print "ROI           : %dx%d at %d,%d" %(width, height, x, y)
print "Payload       : %d" %(payload)
print "Pixel format  : %s" %(camera.get_pixel_format_as_string ())

stream = camera.create_stream (None, None)

stream.push_buffer (Aravis.Buffer.new_allocate (payload))

print "Start acquisition"

camera.start_acquisition ()

print "Acquisition"

buffer = stream.pop_buffer ()

data = buffer.get_data ()

DumpBuffer (data, len(data), "Image buffer")

print "Stop acquisition"

camera.stop_acquisition ()

