Title: Introduction

# What is Aravis ?

Aravis is a GObject based library for the control and the video stream
acquisition of digital cameras.

While using GenICam xml files for the description of the camera registers, it
does not try to be a complete implementation of the Genicam API as described in
the GenAPI GenICam document. Nevertheless, a good knowledge of the
[GenICam](http" url="http://www.genicam.org) standard should help to understand
how Aravis operates.

Aravis currently provides an implementation of the gigabit ethernet and USB3
protocols found in a lot of ethernet industrial cameras.

[class@Aravis.Camera] is a simple API for easy access of standard camera features.

[class@Aravis.Device] and [class@Aravis.Gc] are more low level APIs, which
enable the full control of the cameras, allowing the use of the features
specific to each model.
