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

### Documentation

The latest documentation is available
[here](https://aravisproject.github.io/aravis). You will find how to install
Aravis on Linux, macOS and Windows, how to tweak your system in order to get the
best performances, and the API documentation.

### Dependencies

The Aravis library depends on zlib, libxml2 and glib2, with an optional USB
support depending on libusb1.

The GStreamer plugin depends on GStreamer1 in addition to the Aravis library
dependencies.

The simple viewer depends on GStreamer1, Gtk+3 and the Aravis library
dependencies.

The required versions are specified in the
[meson.build](https://github.com/AravisProject/aravis/blob/main/meson.build)
file in Aravis sources.

It is perfectly possible to only build the library, reducing the dependencies to
the bare minimum.

### Contributions

As an open source and free software project, we welcome any contributions to the
aravis project: code, bug reports, testing...

However, contributions to both Gigabit Ethernet and USB3 protocol code (files
`src/arvuv*.[ch]` `src/arvgv*.[ch]`) must not be based on the corresponding
specification documents published by the [A3](https://www.automate.org/vision), as
this organization forbids the use of their documents for the development of an
open source implementation of the specifications. So, if you want to contribute
to this part of Aravis, don't use the A3 documents and state clearly in the
pull request your work is not based on them.

### Links

* Forum: https://aravis-project.discourse.group
* Github repository: https://github.com/AravisProject/aravis
* Releases: https://github.com/AravisProject/aravis/releases
* Release notes: https://github.com/AravisProject/aravis/blob/master/NEWS.md
* Aravis 0.8 documentation: https://aravisproject.github.io/docs/aravis-0.8/
* Genicam standard : http://www.genicam.org
