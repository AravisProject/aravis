#!/bin/sh

mkdir -p "$2/drivers/kmod.dir"
cp -Rf   "$1/drivers/"* "$2/drivers/kmod.dir"
rm -f    "$2/drivers/kmod.dir/meson.build" "$2/drivers/kmod.dir/mesonbuild.sh"
cd       "$2/drivers/kmod.dir"
sed -i -e "s|\$(src)|$1/drivers|" "$2/drivers/kmod.dir/Kbuild"
make
mv       "aravis_gv.ko" "$2/drivers"
