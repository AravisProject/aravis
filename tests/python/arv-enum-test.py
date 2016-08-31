#!/usr/bin/env python

#  If you have installed aravis in a non standard location, you may need
#   to make GI_TYPELIB_PATH point to the correct location. For example:
#
#   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/
#
#  You may also have to give the path to libaravis.so, using LD_PRELOAD or
#  LD_LIBRARY_PATH.

import gi

gi.require_version ('Aravis', '0.2')

from gi.repository import Aravis

print Aravis.Auto
print Aravis.Auto.OFF
print Aravis.BufferStatus
print Aravis.DebugLevel
print Aravis.DomNodeType
print Aravis.GvStreamPacketResend
print Aravis.GvspPacketType

print Aravis.PIXEL_FORMAT_MONO_8
