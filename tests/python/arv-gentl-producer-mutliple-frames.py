#!/usr/bin/env python3
"""Acquire 1000 frames through the Aravis GenTL Producer."""

import os
import time
from pathlib import Path

from harvesters.core import Harvester

FRAME_COUNT = 1000


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
    acquired = 0
    start_time = time.monotonic()
    try:
        harvester.add_file(str(cti), check_existence=True, check_validity=True)
        harvester.update()
        if not harvester.device_info_list:
            raise RuntimeError("No device found")

        print(f"Device: {harvester.device_info_list[0]}")
        acquirer = harvester.create(0)
        acquirer.start()
        started = True
        start_time = time.monotonic()

        for index in range(FRAME_COUNT):
            with acquirer.fetch(timeout=5.0) as buffer:
                if not buffer.module.is_complete:
                    raise RuntimeError(f"Frame {index} is incomplete")
                if not buffer.payload.components:
                    raise RuntimeError(f"Frame {index} has no image component")
                if buffer.payload.components[0].data.size == 0:
                    raise RuntimeError(f"Frame {index} is empty")
                acquired += 1

            if acquired % 100 == 0:
                elapsed = time.monotonic() - start_time
                print(f"  {acquired}/{FRAME_COUNT} frames ({acquired / elapsed:.1f} fps)")
    finally:
        if started:
            acquirer.stop()
        if acquirer is not None:
            acquirer.destroy()
        harvester.reset()

    elapsed = time.monotonic() - start_time
    print(f"Acquired {acquired}/{FRAME_COUNT} frames in {elapsed:.1f} s")


if __name__ == "__main__":
    main()
