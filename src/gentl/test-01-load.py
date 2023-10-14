from harvesters.core import Harvester
H=Harvester()
H.add_file('../../build/src/gentl/aravis-0.8.30.cti')
H.update()
print(H.device_info_list)
