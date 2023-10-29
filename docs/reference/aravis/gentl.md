Title: GenTL Devices

# GenTL

Aravis can load camera vendors' GenTL producers to connect GenICam cameras.
It searches GenTL producers, dynamic libraries with *.cti* extension,
in the paths defined by environment variable `GENICAM_GENTL{32/64}_PATH`.
Each path can be either a directory or a file.