project ('aravis', 'c', 'cpp', version: '0.6.1', meson_version: '>=0.45.0')

gnome = import('gnome')
i18n = import ('i18n')
pkg = import ('pkgconfig')

aravis_version = meson.project_version ()
aravis_api_version = '0.6'

versions = aravis_version.split ('.')
aravis_major_version = versions[0].to_int ()
aravis_minor_version = versions[1].to_int ()
aravis_micro_version = versions[2].to_int ()

aravis_desktop_dir = join_paths (get_option ('datadir'), 'applications')
aravis_data_dir = join_paths (get_option ('datadir'), 'aravis-@0@'.format (aravis_api_version))
aravis_icon_dir = join_paths (aravis_data_dir, 'icons', 'hicolor')

cc = meson.get_compiler ('c')
cxx = meson.get_compiler ('cpp')

glib_dep = dependency ('glib-2.0', version: '>=2.32', required: true)
gobject_dep = dependency ('gobject-2.0', required: true)
gio_dep = dependency ('gio-2.0', required: true)
xml2_dep = dependency ('libxml-2.0', required: true)
libz_dep = dependency ('zlib', required: true)

libm = cc.find_library ('m', required: true)

aravis_dependencies = [glib_dep, gobject_dep, gio_dep, xml2_dep, libz_dep, libm]
aravis_dependency_str = 'glib-2.0 gobject-2.0 gio-2.0'
aravis_private_dependency_str = 'libxml-2.0 zlib'

enable_packet_socket = get_option ('packet-socket')
if enable_packet_socket
	aravis_dependencies += dependency ('audit', required: true)
	aravis_private_dependency_str += ' audit'
endif

enable_usb = get_option ('usb')
if enable_usb
	aravis_dependencies += dependency ('libusb-1.0', required: true)
	aravis_private_dependency_str += ' libusb-1.0'
endif

config_data = configuration_data ()
config_data.set_quoted ('PACKAGE_NAME', 'aravis')
config_data.set_quoted ('PACKAGE_VERSION', aravis_version)
config_data.set_quoted ('PACKAGE', 'aravis')
config_data.set_quoted ('VERSION', aravis_version)
config_data.set ('ARAVIS_BUILD_PACKET_SOCKET', enable_packet_socket)
config_data.set ('ARAVIS_BUILD_USB', enable_usb)
config_data.set ('ARAVIS_FAST_HEARTBEAT', get_option ('fast-heartbeat'))
configure_file (input: 'arvconfig.h.in', output: 'arvconfig.h', configuration: config_data)
configuration_inc = include_directories ('.')

subdir ('po')
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

	subdir ('viewer')
endif

if get_option ('gst-plugin')
	gstreamer_base_dep = dependency ('gstreamer-base-1.0', required: true)
	gstreamer_app_dep = dependency ('gstreamer-app-1.0', required: true)

	gst_plugin_dependencies = [aravis_dependencies, gstreamer_base_dep]

	subdir ('gst')
endif

if get_option ('documentation')
	subdir ('docs/reference/aravis')
endif
