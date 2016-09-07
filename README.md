# ![](viewer/icons/gnome/256x256/apps/aravis.png) Aravis

### What is Aravis ?

Aravis is a glib/gobject based library for video acquisition using Genicam cameras. It currently implements the gigabit ethernet and USB3 (in aravis 0.5.x)  protocols used by industrial cameras. It also provides a basic ethernet camera simulator and a simple video viewer.

# ![](viewer/data/aravis.png)
# ![](viewer/data/aravis-video.png)

Aravis is released under the LGPL v2+.

### Dependencies

The Aravis library depends on libxml2 and glib2, with an optional USB support depending on libusb1.

The GStreamer plugin depends on GStreamer1 in addition to the Aravis library dependencies.

The simple viewer depends on GStreamer1, Gtk+3, libnotify and the Aravis library dependencies.

The required versions are specified in the [configure.ac](https://github.com/AravisProject/aravis/blob/master/configure.ac#L67) file in Aravis sources.

### Downloads

* 0.4.x stable releases: http://ftp.gnome.org/pub/GNOME/sources/aravis/0.4/
* 0.5.x development releases: http://ftp.gnome.org/pub/GNOME/sources/aravis/0.5/

### Links

* Github repository: https://github.com/AravisProject/aravis
* Mailing list: aravis@freelists.org ( http://www.freelists.org/list/aravis )
* Documentation: https://aravisproject.github.io/docs/aravis-0.4/
* Blog: http://blogs.gnome.org/emmanuel/category/aravis/
* Genicam standard : http://www.genicam.org
