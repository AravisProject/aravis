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

let evaluator = Aravis.Evaluator.new ("1+2*4.4");

let intResult = evaluator.evaluate_as_int64 ();
let dblResult = evaluator.evaluate_as_double ();

print (intResult);
print (dblResult);

evaluator.set_expression ("VAR+10");
evaluator.set_double_variable ("VAR", 1.2);

dblResult = evaluator.evaluate_as_double ();

print (dblResult);
