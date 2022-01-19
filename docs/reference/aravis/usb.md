Title: USB Devices

# USB

## Permissions

By default, USB devices permissions may not be sufficient to allow any user to
access the USB3 cameras. These permissions can be changed by using an udev rule
file. There is a file example in [Aravis
sources](https://github.com/AravisProject/aravis/blob/main/src/aravis.rules).
This file must be placed in `/etc/udev/rules.d` directory (The exact location
may depend on the distribution you are using). This file only contains
declarations for a couple of vendors. If you want to add an entry with the
vendor of your camera, the output of `lsusb` command will give you the vendor
id, which is the first 4 digits of the ID field.

## Performance

Aravis uses by default the synchronous libusb API. But it can be told to use the
asynchronous API for better performances, especially on embedded platform like
RapsberryPi or Nvidia Jetson boards. The function to use is
[method@Aravis.Camera.uv_set_usb_mode].

`arv-viewer` and `arv-camera-test` can use the asynchronous API if `usb-mode`
option is set to `async`. Similarly, the GStreamer plugin is using the
asynchronous API if `usb-mode` property is set to `async`.
