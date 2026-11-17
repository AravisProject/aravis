Title: GStreamer plugin

# GStreamer plugin

Its is possible to optionally build a GStreamer plugin that can connects to the
camera supported by Aravis.

The plugin implements a aravissrc source element. Here are the properties of the
element:

```
Factory Details:
  Rank                     none (0)
  Long-name                Aravis Video Source
  Klass                    Source/Video
  Description              Aravis based source
  Author                   Emmanuel Pacaud <emmanuel.pacaud@free.fr>

Plugin Details:
  Name                     aravis
  Description              Aravis Video Source
  Filename                 /home/pacaud/Sources/aravis/main/build/gst/libgstaravis.0.10.so
  Version                  0.9.3
  License                  LGPL
  Source module            aravis
  Binary package           aravis
  Origin URL               http://blogs.gnome.org/emmanuel

GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseSrc
                         +----GstPushSrc
                               +----GstAravis

Element Flags:
  - SOURCE

Pad Templates:
  SRC template: 'src'
    Availability: Always
    Capabilities:
      ANY

Element has no clocking capabilities.
Element has no URI handling capabilities.

Pads:
  SRC: 'src'
    Pad Template: 'src'

Element Properties:

  add-reference-timestamp-meta: Add a GstReferenceTimestampMeta with timestamp/x-ptp reference caps to buffers, carrying the device timestamp. Only an absolute time if the device clock is PTP-synchronized
                        flags: accès en lecture, accès en écriture
                        Boolean. Default: false
  
  auto-packet-size    : Negotiate GigEVision streaming packet size
                        flags: accès en lecture, accès en écriture
                        Boolean. Default: false
  
  automatic-eos       : Automatically EOS when the segment is done
                        flags: accès en lecture, accès en écriture
                        Boolean. Default: true
  
  blocksize           : Size in bytes to read per buffer (-1 = default)
                        flags: accès en lecture, accès en écriture
                        Unsigned Integer. Range: 0 - 4294967295 Default: 4096 
  
  camera              : Camera instance to retrieve additional information
                        flags: accès en lecture
                        Object of type "ArvCamera"
  
  camera-name         : Name of the camera
                        flags: accès en lecture, accès en écriture
                        String. Default: null
  
  do-timestamp        : Apply current stream time to buffers
                        flags: accès en lecture, accès en écriture
                        Boolean. Default: false
  
  exposure            : Exposure time (µs)
                        flags: accès en lecture, accès en écriture
                        Double. Range:              -1 -           1e+08 Default:              -1 
  
  exposure-auto       : Auto Exposure Mode
                        flags: accès en lecture, accès en écriture
                        Enum "GstArvAuto" Default: 0, "off"
                           (0): off              - Off
                           (1): once             - Once
                           (2): on               - Continuous
  
  features            : Additional configuration parameters as a space separated list of feature assignations
                        flags: accès en lecture, accès en écriture
                        String. Default: null
  
  gain                : Gain (dB)
                        flags: accès en lecture, accès en écriture
                        Double. Range:              -1 -             500 Default:              -1 
  
  gain-auto           : Auto Gain Mode
                        flags: accès en lecture, accès en écriture
                        Enum "GstArvAuto" Default: 0, "off"
                           (0): off              - Off
                           (1): once             - Once
                           (2): on               - Continuous
  
  gamma               : Gamma
                        flags: accès en lecture, accès en écriture
                        Double. Range:               0 -             3,9 Default:               1 
  
  h-binning           : CCD horizontal binning
                        flags: accès en lecture, accès en écriture
                        Integer. Range: 1 - 2147483647 Default: -1 
  
  name                : The name of the object
                        flags: accès en lecture, accès en écriture
                        String. Default: "aravis0"
  
  num-arv-buffers     : Number of video buffers to allocate for video frames
                        flags: accès en lecture, accès en écriture
                        Integer. Range: 1 - 2147483647 Default: 50 
  
  num-buffers         : Number of buffers to output before sending EOS (-1 = unlimited)
                        flags: accès en lecture, accès en écriture
                        Integer. Range: -1 - 2147483647 Default: -1 
  
  offset-x            : Offset in x direction
                        flags: accès en lecture, accès en écriture
                        Integer. Range: 0 - 2147483647 Default: 0 
  
  offset-y            : Offset in y direction
                        flags: accès en lecture, accès en écriture
                        Integer. Range: 0 - 2147483647 Default: 0 
  
  packet-delay        : GigEVision streaming inter packet delay (in ns, -1 = default)
                        flags: accès en lecture, accès en écriture
                        Integer64. Range: 0 - 9223372036 Default: -1 
  
  packet-resend       : Request dropped packets to be reissued by the camera
                        flags: accès en lecture, accès en écriture
                        Boolean. Default: true
  
  packet-size         : GigEVision streaming packet size
                        flags: accès en lecture, accès en écriture
                        Integer. Range: 36 - 65500 Default: -1 
  
  parent              : The parent of the object
                        flags: accès en lecture, accès en écriture
                        Object of type "GstObject"
  
  stream              : Stream instance to retrieve additional information
                        flags: accès en lecture
                        Object of type "ArvStream"
  
  timeout-policy      : How empty buffer waits are handled: error or wait
                        flags: accès en lecture, accès en écriture
                        Enum "GstAravisTimeoutPolicy" Default: 0, "error"
                           (0): error            - Error
                           (1): wait             - Wait
  
  timing-mode         : Whether the source timing is fixed-rate or sparse/irregular
                        flags: accès en lecture, accès en écriture
                        Enum "GstAravisTimingMode" Default: 0, "fixed"
                           (0): fixed            - Fixed
                           (1): sparse           - Sparse
  
  trigger             : Enable the trigger mode using the given source
                        flags: accès en lecture, accès en écriture
                        String. Default: null
  
  typefind            : Run typefind before negotiating (deprecated, non-functional)
                        flags: accès en lecture, accès en écriture, obsolète
                        Boolean. Default: false
  
  usb-mode            : USB mode (synchronous/asynchronous)
                        flags: accès en lecture, accès en écriture
                        Enum "GstArvUsbMode" Default: 1, "default"
                           (0): sync             - Synchronous
                           (1): async            - Asynchronous
                           (1): default          - Default
  
  v-binning           : CCD vertical binning
                        flags: accès en lecture, accès en écriture
                        Integer. Range: 1 - 2147483647 Default: -1 
  

Element Actions:

  "software-trigger" -> void  :  g_signal_emit_by_name (element, "software-trigger");
```

The plugin must be installed to be usable by a GStreamer pipeline.
Alternatively, the GST_PLUGIN_PATH environment variable allows to tell GStreamer
where to find the Aravis plugin.

You can also test the uninstalled plugin using `gst-aravis-launch`
and`gst-aravis-inspect` helper scripts build in `builddir/gst/`.

A simple pipeline would be:

```
gst-launch-1.0 aravissrc ! videoconvert ! autovideosink
```

It will connect to the first available camera and display the video in a
window.

Using a format filter, you can select the video format and/or the framerate:

```
gst-launch-1.0 aravissrc ! video/x-raw,format=GRAY8,framerate=20/1 ! videoconvert ! autovideosink
```
