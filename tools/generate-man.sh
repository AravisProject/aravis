export LANG=C

version="0.6"

help2man -n "Small utility for basic control of Genicam devices" --version-string="${version}" src/arv-tool-${version} > ../man/arv-tool-${version}.1
help2man -n "Basic viewer for Aravis compatible cameras" --version-string="${version}" viewer/arv-viewer-${version} > ../man/arv-viewer-${version}.1

