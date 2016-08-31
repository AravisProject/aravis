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

buffer_a = Aravis.Buffer.new_allocate (1024)
buffer_b = Aravis.Buffer.new_allocate (1024)

print "Buffer a refcount :        %d" %(buffer_a.__grefcount__)
print "Buffer a is preallocated : %d" %(buffer_a.is_preallocated)
print "Buffer b refcount :        %d" %(buffer_b.__grefcount__)
print "Buffer b is preallocated : %d" %(buffer_b.is_preallocated)

