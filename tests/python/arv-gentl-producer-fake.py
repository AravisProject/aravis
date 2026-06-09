#!/usr/bin/env python3
"""Acquire one frame from the Aravis Fake camera through the GenTL Producer."""

import os
from pathlib import Path

import gi

gi.require_version("Aravis", "0.10")
from gi.repository import Aravis  # noqa: E402

from harvesters.core import Harvester  # noqa: E402


def get_cti_path() -> Path:
    default = Path(__file__).parents[2] / "build/src/gentl/ProducerAravis-0.10.cti"
    return Path(os.environ.get("ARAVIS_CTI", default)).resolve()


def main() -> None:
    root = Path(__file__).parents[2]
    cti = get_cti_path()
    if not cti.is_file():
        raise FileNotFoundError(f"GenTL producer not found: {cti}")

    Aravis.enable_interface("Fake")
    Aravis.set_fake_camera_genicam_filename(str(root / "src/arv-fake-camera.xml"))

    print(f"Using CTI: {cti}")
    harvester = Harvester()
    acquirer = None
    started = False
    try:
        harvester.add_file(str(cti), check_existence=True, check_validity=True)
        harvester.update()
        devices = [
            device
            for device in harvester.device_info_list
            if device.property_dict.get("tl_type") == "GEV"
            and device.property_dict.get("model") == "Fake"
        ]
        if not devices:
            raise RuntimeError("The Fake camera was not announced")

        print(f"Opening device: {devices[0]}")
        acquirer = harvester.create(devices[0])
        acquirer.start()
        started = True

        with acquirer.fetch(timeout=5.0) as buffer:
            if not buffer.module.is_complete:
                raise RuntimeError("The acquired buffer is incomplete")
            if not buffer.payload.components:
                raise RuntimeError("The acquired buffer has no image component")
            component = buffer.payload.components[0]
            if component.data.size == 0:
                raise RuntimeError("The acquired image is empty")
            print(
                f"Acquired {component.width}x{component.height} "
                f"{component.data_format} frame"
            )
    finally:
        if started:
            acquirer.stop()
        if acquirer is not None:
            acquirer.destroy()
        harvester.reset()


if __name__ == "__main__":
    main()
