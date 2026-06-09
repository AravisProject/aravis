#!/usr/bin/env python3
"""List devices exposed by the Aravis GenTL Producer."""

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
    try:
        harvester.add_file(str(cti), check_existence=True, check_validity=True)
        harvester.update()

        print("Devices:")
        for index, info in enumerate(harvester.device_info_list):
            print(f"  {index}: {info}")
        print(f"\nTotal: {len(harvester.device_info_list)} device(s)")
    finally:
        harvester.reset()


if __name__ == "__main__":
    main()
