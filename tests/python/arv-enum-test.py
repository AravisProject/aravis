#!/usr/bin/env python

# Aravis - Digital camera library
#
# Copyright (c) 2011-2012 Emmanuel Pacaud
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
