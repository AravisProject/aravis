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

let device = Aravis.FakeDevice.new ("TEST0");
let genicam = device.get_genicam ();

let payload = genicam.get_node ("PayloadSize").get_value ();
let width = genicam.get_node ("SensorWidth").get_value ();
let height = genicam.get_node ("SensorHeight").get_value ();

print ("Payload : ", payload);
print ("Width   : ", width);
print ("Height  : ", height);
