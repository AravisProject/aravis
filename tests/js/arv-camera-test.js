#!/usr/bin/env gjs

/* SPDX-License-Identifier:Unlicense */

/*
   If you have installed aravis in a non standard location, you may need
   to make GI_TYPELIB_PATH point to the correct location. For example:

   export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/opt/bin/lib/girepositry-1.0/

   You may also have to give the path to libaravis.so, using LD_PRELOAD or
   LD_LIBRARY_PATH.
 */

const GLib = imports.gi.GLib;
const Aravis = imports.gi.Aravis;

let camera = Aravis.Camera.new (null);

camera.set_region (0,0,128,128);
camera.set_pixel_format (Aravis.PIXEL_FORMAT_MONO_8);
camera.set_frame_rate (10.0);

let [x,y,width,height] = camera.get_region ();
let payload = camera.get_payload ();

print ("Camera vendor : ", camera.get_vendor_name ());
print ("Camera model  : ", camera.get_model_name ());
print ("ROI           : ", width, "x", height, " at ", x, ",", y);
print ("Payload       : ", payload);
print ("Pixel format  : ", camera.get_pixel_format_as_string ());

let stream = camera.create_stream (null);

for (let i = 0; i < 10; i++) {
    stream.push_buffer (Aravis.Buffer.new_allocate (payload));
}

print ("Start Acquisition");

camera.start_acquisition ();

print ("Acquisition");

for (let i = 0; i < 20; i++) {
    let buffer = stream.pop_buffer ();
    print (buffer);
    if (buffer) {
	stream.push_buffer (buffer);
    }
}

print ("Stop acquisition");

camera.stop_acquisition ();
