### Building Aravis for Windows

Aravis can be experimentally built using the MinGW cross-compiler suite on Linux host.

1. clone aravis sources `git clone https://github.com/AravisProject/aravis.git` (we suppose that the sources will be in `~/aravis` in the following).

1. install [crossroad](https://pypi.org/project/crossroad/) with `pip3 install crossroad`.

2. create cross-compilation environment (called `test`) and enter via `crossroad w64 test1`:

```
You are now at the crossroads...

Your environment has been set to cross-compile the project 'test1' on Windows 64-bit (w64).
Use `crossroad help` to list available commands and `man crossroad` to get a full documentation of crossroad capabilities.
To exit this cross-compilation environment, simply `exit` the current shell session.
w64✘test1 host:~ $
```

3. install dependencies (*must* be done in the cross-shell):

```
crossroad install libxml2 glib2 libnotify libusb gtk3 gstreamer gst-plugins-base gst-plugins-good gobject-introspection
```

3. configure the build (*must* be done in the cross-shell):

```
crossroad meson ~/aravis ~/aravis-build -Dbuildtype=release -Ddocumentation=disabled -Dintrospection=disabled -Dgst-plugin=enabled -Dusb=enabled -Dviewer=enabled
```

4. Run the build and installation (*can* be done in the cross-shell, but also in regular shell):

```
ninja -C ~/aravis-build install
```

5. Test arv-tool via Wine (exporting the WINEDLLPATH does not seem to be necessary, though):

```
cd ~/.local/share/crossroad/roads/w64/test1/bin/
export WINEDLLPATH=~/.local/share/crossroad/roads/w64/test1/lib
wine arv-tool-0.8.exe
```

6. The entire `.local/share/crossroad/roads/w64/test1` directory can be exported over the network (such as via VirtualBox's shared drive), mounted as e.g. `Z:` and binaries run natively.

7. `arv-viewer` needs some extra setup because of the GUI.

  a) Running `gdk-pixbuf-query-loaders.exe --update-cache` once was necessary so that GDK would not abort locating pixmap loading plugins.

  b) set `XDG_DATA_DIRS` so that icons can be found.

     ```
     # Wine
     export XDG_DATA_DIRS=~/.local/share/crossroad/roads/w64/test1/share
     wine arv-viewer-0.8.exe
     # Windows native
     export XDG_DATA_DIRS=Z:/share
     z:
     cd bin
     arv-viewer-0.8.exe
     ```


## Issues

* Some features will not build as the default cross-build does not support running host binaries directly: `documentation` and `introspection`.

* Camera discovery is currently not functional (investigation of the issue is ongoing).
