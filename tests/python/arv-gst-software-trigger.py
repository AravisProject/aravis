#!/usr/bin/env python3

# SPDX-License-Identifier:Unlicense

#  If you have installed aravis in a non standard location, you may need
#   to make GI_TYPELIB_PATH point to the correct location. For example:
#
#   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/
#
#  You may also have to give the path to libaravis.so, using LD_PRELOAD or
#  LD_LIBRARY_PATH.
#
# As this example alos uses the aravis gstreamer plugin, you may need to tell the path to the plugin using
# GST_PLUGIN_PATH.

# Example of a gstreamer pipeline using a software trigger
#
# Author: WhaSukGO <leeju213@gmail.com>

import gi
import threading
import time

gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

# Initialize GStreamer
Gst.init(None)

def main():

    # Create GStreamer elements
    aravissrc = Gst.ElementFactory.make("aravissrc", "source")
    aravissrc.set_property("exposure", 1700)
    aravissrc.set_property("do-timestamp", True)
    aravissrc.set_property("trigger", "Software")

    bayer_caps = Gst.Caps.from_string("video/x-bayer,format=rggb,width=800,height=600")
    bayer2rgb = Gst.ElementFactory.make("bayer2rgb", "bayer2rgb")

    videoconvert = Gst.ElementFactory.make("videoconvert", "videoconvert")

    videosink = Gst.ElementFactory.make("autovideosink", "videosink")

    # Create a new pipeline
    pipeline = Gst.Pipeline.new("mypipeline")

    # Add elements to the pipeline
    pipeline.add(aravissrc)
    pipeline.add(bayer2rgb)
    pipeline.add(videoconvert)
    pipeline.add(videosink)

    # Link the elements with the capsfilter in between
    aravissrc.link_filtered(bayer2rgb, bayer_caps)
    bayer2rgb.link(videoconvert)
    videoconvert.link(videosink)

    # Function to trigger the camera
    def trigger_camera():
        while True:
            time.sleep(0.5)  # Sleep for 2 milliseconds (500fps)
            aravissrc.emit("software-trigger")

    # Create and start the trigger thread
    trigger_thread = threading.Thread(target=trigger_camera)
    trigger_thread.daemon = True  # Daemonize thread so it exits when the main program does
    trigger_thread.start()

    # Start the pipeline
    pipeline.set_state(Gst.State.PLAYING)

    # Run the pipeline
    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass

    # Clean up
    pipeline.set_state(Gst.State.NULL)

if __name__ == "__main__":
    main()
