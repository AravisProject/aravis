project ('aravis', 'c', 'cpp', version: '0.7.5', meson_version: '>=0.47.0')

gnome = import('gnome')
pkg = import ('pkgconfig')

aravis_version = meson.project_version ()
aravis_api_version = '0.8'

versions = aravis_version.split ('.')
aravis_major_version = versions[0].to_int ()
aravis_minor_version = versions[1].to_int ()
aravis_micro_version = versions[2].to_int ()

aravis_data_dir = join_paths (get_option ('datadir'), 'aravis-@0@'.format (aravis_api_version))

cc = meson.get_compiler ('c')
cxx = meson.get_compiler ('cpp')

glib_dep = dependency ('glib-2.0', version: '>=2.44', required: true)
gobject_dep = dependency ('gobject-2.0', required: true)
gio_dep = dependency ('gio-2.0', required: true)
xml2_dep = dependency ('libxml-2.0', required: true)
libz_dep = dependency ('zlib', required: true)

libm = cc.find_library ('m', required: true)

aravis_public_dependencies = [glib_dep, gobject_dep, gio_dep]
aravis_dependencies = [aravis_public_dependencies, xml2_dep, libz_dep, libm]

enable_usb = get_option ('usb')
if enable_usb
	aravis_dependencies += dependency ('libusb-1.0', required: true)
endif

subdir ('src')
subdir ('tests')

if get_option ('viewer')
	gtk_dep = dependency ('gtk+-3.0', version: '>=3.12', required: true)
	gstreamer_base_dep = dependency ('gstreamer-base-1.0', required: true)
	gstreamer_app_dep = dependency ('gstreamer-app-1.0', required: true)
	gstreamer_video_dep = dependency ('gstreamer-video-1.0', required: true)
	notify_dep = dependency ('libnotify', required: true)

	aravis_viewer_dependencies = [aravis_dependencies, gtk_dep, gstreamer_base_dep, gstreamer_app_dep,
		gstreamer_video_dep, notify_dep]

	i18n = import ('i18n')

	subdir ('po')
	subdir ('viewer')
endif

if get_option ('gst-plugin')
	gstreamer_base_dep = dependency ('gstreamer-base-1.0', required: true)
	gstreamer_app_dep = dependency ('gstreamer-app-1.0', required: true)

	gst_plugin_dependencies = [aravis_dependencies, gstreamer_base_dep]

	gst_plugin_dir = join_paths (get_option ('libdir'), 'gstreamer-1.0')

	subdir ('gst')
endif

subdir ('docs')
