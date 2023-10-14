from harvesters.core import Harvester
H=Harvester()
H.add_file('../../build/src/gentl/aravis-0.8.30.cti')
H.update()
from rich.pretty import pprint
pprint(H.device_info_list)
