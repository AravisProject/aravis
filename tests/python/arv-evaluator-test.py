#!/usr/bin/env python

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
