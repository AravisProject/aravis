#!/usr/bin/env python3
"""Acquire one frame through the Aravis GenTL Producer."""

import os
from pathlib import Path

from harvesters.core import Harvester


def get_cti_path() -> Path:
    default = Path(__file__).parents[2] / "build/src/gentl/ProducerAravis-0.10.cti"
    return Path(os.environ.get("ARAVIS_CTI", default)).resolve()


def main() -> None:
    cti = get_cti_path()
    if not cti.is_file():
        raise FileNotFoundError(f"GenTL producer not found: {cti}")

    print(f"Using CTI: {cti}")
    harvester = Harvester()
    acquirer = None
    started = False
    try:
        harvester.add_file(str(cti), check_existence=True, check_validity=True)
        harvester.update()
        if not harvester.device_info_list:
            raise RuntimeError("No device found")

        print(f"Opening device 0: {harvester.device_info_list[0]}")
        acquirer = harvester.create(0)
        acquirer.start()
        started = True

        with acquirer.fetch(timeout=5.0) as buffer:
            if not buffer.payload.components:
                raise RuntimeError("The acquired buffer has no image component")
            component = buffer.payload.components[0]
            if component.data.size == 0:
                raise RuntimeError("The acquired image is empty")
            print(f"  width:       {component.width}")
            print(f"  height:      {component.height}")
            print(f"  data format: {component.data_format}")
            print(f"  shape:       {component.data.shape}")
            print(f"  dtype:       {component.data.dtype}")
            print(f"  data size:   {component.data.size} elements")
    finally:
        if started:
            acquirer.stop()
        if acquirer is not None:
            acquirer.destroy()
        harvester.reset()


if __name__ == "__main__":
    main()
