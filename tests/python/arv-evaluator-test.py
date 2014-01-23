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

from gi.repository import Aravis

evaluator = Aravis.Evaluator.new ("1+2*4.4")

int_result = evaluator.evaluate_as_int64 ()
dbl_result = evaluator.evaluate_as_double ()

print int_result
print dbl_result

evaluator.set_expression ("VAR+10")
evaluator.set_double_variable ("VAR", 1.2)

dbl_result = evaluator.evaluate_as_double()

print dbl_result
