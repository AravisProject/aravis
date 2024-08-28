from harvesters.core import Harvester
import os

H=Harvester()
H.add_file(os.environ.get('ARV_PRODUCER_PATH'))
H.update()

from rich.pretty import pprint

pprint(H.device_info_list)
ia=H.create(1)
print('Width:',ia.remove_device.node_map.Width.value)
