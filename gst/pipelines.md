Simple
======

./gst-aravis-launch aravissrc ! videoconvert ! xvimagesink

Format filter
=============

./gst-aravis-launch aravissrc ! video/x-raw,format=GRAY16_LE,depth=12 ! videoconvert ! xvimagesink
