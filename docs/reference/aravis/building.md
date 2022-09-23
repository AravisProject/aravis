Title: Installation and Debug

# Installing Aravis

Aravis uses the meson build system ( http://mesonbuild.com/ ). After you have
downloaded the latest release from
[https://github.com/AravisProject/aravis/releases](https://github.com/AravisProject/aravis/releases),
you can build and install Aravis like [any other meson
project](http://mesonbuild.com/Quick-guide.html#compiling-a-meson-project):

```sh
meson build
cd build
ninja
ninja install
```

The build can be configured at any time using `meson configure` in the build
directory. `meson configure` invoked without any other argument will show the
configuration options.

On some platforms (like Ubuntu), you may have to configure the dynamic linker
(ld) to let it know where the aravis libraries are installed, and run ldconfig
as root in order to update ld cache.

## Install dependencies on Ubuntu 20.04

Prior to running `meson` and `ninja`, dependencies can be installed using the
following(tested on Ubuntu 20.04):

```sh
sudo apt install libxml2-dev libglib2.0-dev cmake libusb-1.0-0-dev gobject-introspection \
                 libgtk-3-dev gtk-doc-tools  xsltproc libgstreamer1.0-dev \
                 libgstreamer-plugins-base1.0-dev libgstreamer-plugins-good1.0-dev \
                 libgirepository1.0-dev gettext
```

## Install dependencies on Fedora 34/35
Due to differences in the deb and rpm package ecosystems dependencies can be
installed on  Fedora (tested on 34 and 35) with:

```sh
sudo dnf install libxml2-devel glib2-devel cmake libusb1-devel gobject-introspection \
                 gobject-introspection-devel gstreamer1-plugins-base-devel gtk3-devel \
                 gtk-doc libxslt gstreamer1-devel gstreamer1-plugins-good python3-gobject \
                 g++ meson gettext
```

## Building on macOS

Using the GNU build system on macOS is not directly supported, but can be
mimicked by augmenting the install procedure above with some environment
settings (tested on macOS Catalina):

```sh
brew install gettext intltool gtk-doc libxml2 meson libusb
meson build
ninja -C build
```

If you want to be able to build the viewer, you have to install some additional
packages:

```sh
brew install gtk+3 gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad libnotify gnome-icon-theme
meson configure -Dviewer=enabled
```

## Building on Windows

[MSYS2](https://msys2.org) provides native [Aravis
packages](https://packages.msys2.org/base/mingw-w64-aravis).  The package
includes the DLL, headers and utilities (including the viewer).

To build Aravis by yourself, install MSYS2 and enter the mingw64 shell. Refer to
the [mingw CI configuration
file](https://github.com/AravisProject/aravis/blob/main/.github/workflows/aravis-mingw.yml)
for list of dependencies (such as `mingw-w64-x86_64-libxml2` and so on) which
must be installed prior to building via `pacman -S ...`. The build process
itself is the same as on other platforms (meson/ninja).

Alternatively, you can build Aravis using Microsoft Visual C++ (MSVC) and Conan
package manager. Have a look at the [msvc CI configuration
file](https://github.com/AravisProject/aravis/blob/main/.github/workflows/aravis-msvc.yml).

### Cross-compilation for Windows

Aravis for Windows can be also cross-compiled on Linux (and used in Wine) using
[crossroad](https://pypi.org/project/crossroad/), provided that cross-compiler
and native build tools (`sudo apt install gcc-mingw-w64-x86-64 meson
ninja=build` on Debian/Ubuntu) are installed:

```sh
# note: use the git version, not the one from pypi
pip3 install --user git+git://git.tuxfamily.org/gitroot/crossroad/crossroad.git
# create cross-compilation environment with architecture "w64" called "aravis"
crossroad w64 aravis
# install packages required for compilation; crossroad adds the mingw-w64_x86_64- prefix automatically
crossroad install libnotify gstreamer gst-plugins-good gst-plugins-bad gst-plugins-bad gobject-introspection libusb gtk3 libxml2 zlib
# clone aravis sources
git clone https://github.com/AravisProject/aravis
cd aravis
# configure, crossroad adjusts meson for cross-compilation; build directory is created
crossroad meson build
# compile and install
ninja -C build install
```

# Debugging Aravis

The `ARV_DEBUG` environment variable can be set to a comma separated list of
debugging categories, which will make Aravis print out different types of
debugging informations to the console. A debug level can also be specified,
using a number from 0 (none) to 4 (trace) separated from the category name by a
colon. For example, the following command before running an Aravis based
application will make Aravis print out all stream and device related
informations:

```
export ARV_DEBUG=stream:3,device:3
```
Available categories are:

* interface      : Device lookup for each supported protocol
* device         : Device control
* stream         : Video stream management
* stream-thread  : Video stream thread (likely high volume output)
* cp             : Control protocol packets
* sp             : Stream protocol packets (likely high volume output)
* genicam        : Genicam specialized DOM elements
* policies       : Genicam runtime configurable policies
* chunk          : Chunk data code
* dom            : Genicam DOM document
* evaluator      : Expression evaluator
* viewer         : Simple viewer application
* misc           : Miscellaneous code
* all            : Everything


