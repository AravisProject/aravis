<h1 align="center">
  <img src="viewer/icons/gnome/128x128/apps/aravis-0.8.png" alt="Aravis" width="128" height="128"/><br>
  Aravis
</h1>

<p align="center"><strong>Your industrial vision library</strong></p>

[![Aravis-Linux](https://github.com/AravisProject/aravis/actions/workflows/aravis-linux.yml/badge.svg)](https://github.com/AravisProject/aravis/actions/workflows/aravis-linux.yml)
[![Aravis-macOS](https://github.com/AravisProject/aravis/actions/workflows/aravis-macos.yml/badge.svg)](https://github.com/AravisProject/aravis/actions/workflows/aravis-macos.yml)
[![Aravis-MinGW](https://github.com/AravisProject/aravis/actions/workflows/aravis-mingw.yml/badge.svg)](https://github.com/AravisProject/aravis/actions/workflows/aravis-mingw.yml)
[![Aravis-MSVC](https://github.com/AravisProject/aravis/actions/workflows/aravis-msvc.yml/badge.svg)](https://github.com/AravisProject/aravis/actions/workflows/aravis-msvc.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/eaa741156c2041f19b35c336aedf426c)](https://www.codacy.com/gh/AravisProject/aravis/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=AravisProject/aravis&amp;utm_campaign=Badge_Grade)

### What is Aravis ?

Aravis is a glib/gobject based library for video acquisition using Genicam
cameras. It currently implements the gigabit ethernet and USB3 protocols used by
industrial cameras. It also provides a basic ethernet camera simulator and a
simple video viewer.

<p align="center">
  <img src="viewer/data/aravis.png"/>
  <img src="viewer/data/aravis-video.png"/>
</p>

Aravis is released under the LGPL v2+.

### Installing Aravis

Aravis uses the meson build system ( http://mesonbuild.com/ ). After you have
downloaded the latest release from
[https://github.com/AravisProject/aravis/releases](https://github.com/AravisProject/aravis/releases),
you can build and install Aravis like [any other meson
project](http://mesonbuild.com/Quick-guide.html#compiling-a-meson-project):

```
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

#### Building on macOS

Using the GNU build system on macOS is not directly supported, but can be
mimicked by augmenting the install procedure above with some environment
settings (tested on macOS Catalina):

```
brew install gettext intltool gtk-doc libxml2 meson libusb
meson build
ninja -C build
```

If you want to be able to build the viewer, you have to install some additional
packages:

```
brew install gtk+3 gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad libnotify gnome-icon-theme
meson configure -Dviewer=enabled
```

Python bindings and camera simulator are not functional yet.

#### Building on Windows

[MSYS2](https://msys2.org) provides native [Aravis packages](https://packages.msys2.org/base/mingw-w64-aravis).
The package includes the DLL, headers and utilities (including the viewer).

To built Aravis by yourself, install MSYS2 and enter the mingw64 shell. Refer to
[Aravis' PKGBUILD](https://github.com/msys2/MINGW-packages/blob/master/mingw-w64-aravis/PKGBUILD)
for list of dependencies (such as `mingw-w64-x86_64-libxml2` and so on) which must be installed prior
to building via `pacman -S ...`. The build process itself is the same as on other platforms (meson/ninja).

##### Cross-compilation for Windows

Aravis for Windows can be also cross-compiled on Linux (and used in Wine) using [crossroad](https://pypi.org/project/crossroad/), provided that cross-compiler and native build tools (`sudo apt install gcc-mingw-w64-x86-64 meson ninja=build` on Debian/Ubuntu) are installed:

```
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

### Utilities

The main goal of Aravis is to provide a library that interfaces with industrial
cameras. It also provides a set of utilities that help to debug the library,
namely arv-viewer-0.8, arv-tool-0.8, arv-camera-test-0.8 and
arv-fake-gv-camera-0.8. The version suffix corresponds to the API version, as
several stable series of Aravis can be installed at the same time.

The options for each utility is obtained using `--help` argument.

### Ethernet Device Performance

#### Stream Packet Size

One way to increase streaming performance is to increase the stream packet size.
arv_camera_gv_set_packet_size() and arv_camera_gv_auto_packet_size() allow you
to change this parameter. By default, the network adapter of your machine will
probably not let you receive packet bigger than 1500 bytes, which is the default
Maximum Transfer Unit (MTU). It means if you want to use big packets, you also
have to increase the network adapter MTU to a greater walue (8192 bytes being a
recommended value). The exact procedure to change the MTU depends on the
distribution you are using. Please refer to the administrator manual of your
distribution.

On Fedora, MTU can be changed using the network setting panel. Edit the wired
network settings. MTU parameter is in the Identity page.

On Debian / Ubuntu you can add a line "mtu 8192" to /etc/network/interfaces
under the correct device.

```
iface ethusb0 inet static
	address 192.168.45.1
	netmask 255.255.255.0
	broadcast 192.168.45.255
	mtu 8192
```

Please note if your device is not connected directly to the machine, you may
also have to tweak the active devices on your network.

#### Packet Socket Support

Aravis can use packet sockets for the video receiving thread. But this mode
requires extended capabilities. If you want to allow your application to use
packet socket, you must set the `cap_net_raw` capability using `setcap`. For
example, the following command gives this capability to the Aravis viewer (You
should do this on the installed arv-viewer executable, has the one in the source
tree is just a wrapper shell script):

```
sudo setcap cap_net_raw+ep arv-viewer
```

### USB

#### Permissions

By default, USB devices permissions may not be sufficient to allow any user to
access the USB3 cameras. This permissions can be changed by using an udev rule
file. There is a file example in Aravis sources, src/aravis.rules. This file
must be placed in /etc/udev/rules.d directory (The exact location may depend on
the distribution you are using). This file only contains declarations for a
couple of vendors. If you want to add an entry with the vendor of your camera,
the output of `lsusb` command will give you the vendor id, which is the first 4
digits of the ID field.

#### Performance

Aravis uses by default the synchronous libusb API. But it can be told to use the
asynchronous API for better performances, especially on embedded platform like
RapsberryPi or Nvidia Jetson boards. The function to use is
`arv_camera_set_usb_mode()`.

`arv-viewer` and `arv-camera-test` can also use the asynchronous API, if
`usb-mode` option is set to `async`.

### Dependencies

The Aravis library depends on libxml2 and glib2, with an optional USB support
depending on libusb1, and an optional packet socket support depending on
libaudit.

The GStreamer plugin depends on GStreamer1 in addition to the Aravis library
dependencies.

The simple viewer depends on GStreamer1, Gtk+3, libnotify and the Aravis library
dependencies.

The required versions are specified in the
[meson.build](https://github.com/AravisProject/aravis/blob/master/meson.build)
file in Aravis sources.

It is perfectly possible to only build the library, reducing the dependencies to
the bare minimum.

### Contributions

As an open source and free software project, we welcome any contributions to the
aravis project: code, bug reports, testing...

However, contributions to both Gigabit Ethernet and USB3 protocol code (files
`src/arvuv*.[ch]` `src/arvgv*.[ch]`) must not be based on the corresponding
specification documents published by the [AIA](http://www.visiononline.org/), as
this organisation forbids the use of their documents for the development of an
open source implementation of the specifications. So, if you want to contribute
to this part of Aravis, don't use the AIA documents and state clearly in the
pull request your work is not based on them.

#### Unit tests

Aravis has a set of unit tests that helps to catch regressions and memory leaks
during the development. The test suite is run using the following commands:

```
ninja test
```

The is a small helper script that run the same tests under valgrind memmory
checker

```
../tests/valgrind-memcheck
```

All the code is not covered yet by the tests. Code coverage can be obtained
using:

```
meson configure -Db_coverage=true
ninja coverage
```

The report is published in `build/meson-logs/coveragereport/index.html`. Help on
code coverage improvement is welcome.

### Programming examples

While most of the API is documented, Aravis documentation lacks some good
tutorial about the many features if offers. But a good resource is the tests
directory inside Aravis sources, where you will find a set of small samples
showing different key features.

### Porting to Aravis 0.8

Aravis 0.8 has seen a major rewrite of how communication errors are handled.
Instead of relying on a status API, each function that can fail has an
additional error parameter now. This is the standard way of handling error in
the glib ecosytem. The nice side effect is now errors throw exceptions in
bindings where the language support them (rust, python, javascript).

A quick port from the older series to 0.8 series is just a matter of adding a
NULL parameter to most of the modified functions. But you are advised to take
this opportunity to correctly handle errors.

There is a page explaining Glib errors and how to manage them in the [Glib
documentation](https://developer.gnome.org/glib/stable/glib-Error-Reporting.html).

During the camera configuration, in C language it can be somehow cumbersome to
check for errors at each function call. A convenient way to deal with this issue
is the following construction:

```
GError **error = NULL;

if (!error) arv_camera_... (..., &error);
if (!error) arv_camera_... (..., &error);
if (!error) arv_camera_... (..., &error);
if (!error) arv_camera_... (..., &error);

if (error) {
	handle error here;
	g_clear_error (&error);
}
```

### Links

* Forum: https://aravis-project.discourse.group
* Github repository: https://github.com/AravisProject/aravis
* Releases: https://github.com/AravisProject/aravis/releases
* Release notes: https://github.com/AravisProject/aravis/blob/master/NEWS.md
* Aravis 0.8 documentation: https://aravisproject.github.io/docs/aravis-0.8/
* Genicam standard : http://www.genicam.org
