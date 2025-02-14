#!/usr/bin/env python3

# SPDX-License-Identifier:Unlicense

#  If you have installed aravis in a non standard location, you may need
#   to make GI_TYPELIB_PATH point to the correct location. For example:
#
#   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/
#
#  You may also have to give the path to libaravis.so, using LD_PRELOAD or
#  LD_LIBRARY_PATH.

from harvesters.core import Harvester
import os

H=Harvester()
H.add_file(os.environ.get('ARV_PRODUCER_PATH'))
H.update()

from rich.pretty import pprint

pprint(H.device_info_list)
ia=H.create(1)
print('Width:',ia.remove_device.node_map.Width.value)
