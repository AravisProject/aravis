project ('aravis', 'c', 'cpp', version: '0.8.4', meson_version: '>=0.47.0')

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

if cc.get_id()=='gcc' or cc.get_id()=='clang'
  warning_c_args = [
    '-Wduplicated-branches',
    '-Wimplicit-fallthrough',
    '-Wmisleading-indentation',
    '-Wstrict-prototypes',
    '-Wunused',
    '-Wdeclaration-after-statement',
    '-Wformat=2',
    '-Wimplicit-function-declaration',
    '-Winit-self',
    '-Wmissing-include-dirs',
    '-Wmissing-prototypes',
    '-Wpointer-arith',
    ]

  add_project_arguments(cc.get_supported_arguments(warning_c_args), language: 'c')
endif

glib_dep = dependency ('glib-2.0', version: '>=2.44', required: true)
gobject_dep = dependency ('gobject-2.0', required: true)
gio_dep = dependency ('gio-2.0', required: true)
xml2_dep = dependency ('libxml-2.0', required: true)
libz_dep = dependency ('zlib', required: true)
usb_dep = dependency ('libusb-1.0', required: get_option ('usb'))

libm = cc.find_library ('m', required: true)

aravis_public_dependencies = [glib_dep, gobject_dep, gio_dep]
aravis_dependencies = [aravis_public_dependencies, xml2_dep, libz_dep, libm]
if usb_dep.found()
  aravis_dependencies += usb_dep
endif

if host_machine.system()=='windows'
	# socket functions need this library under Windows (winsock2)
	ws2_32 = cc.find_library('ws2_32', required: true)
	aravis_dependencies += ws2_32
endif

has_if_packet = cc.has_header (join_paths ('linux','if_packet.h'))
packet_socket_option = get_option('packet-socket')
if packet_socket_option.enabled()
  if not has_if_packet
    error ('missing header for packet-socket support')
  endif
  packet_socket_enabled = true
else
  packet_socket_enabled = has_if_packet and packet_socket_option.auto()
endif

subdir ('src')
subdir ('tests')

viewer_enabled = false
viewer_option = get_option ('viewer')
viewer_deps = aravis_dependencies + [dependency ('gtk+-3.0', version: '>=3.12', required: viewer_option),
				     dependency ('gstreamer-base-1.0', required: viewer_option),
				     dependency ('gstreamer-app-1.0', required: viewer_option),
				     dependency ('gstreamer-video-1.0', required: viewer_option),
				     dependency ('libnotify', required: viewer_option)]
subdir ('po', if_found: viewer_deps)
subdir ('viewer', if_found: viewer_deps)

gst_option = get_option ('gst-plugin')
gst_deps = aravis_dependencies + [dependency ('gstreamer-base-1.0', required: gst_option),
                                  dependency ('gstreamer-app-1.0', required: gst_option)]
subdir('gst', if_found: gst_deps)

doc_option = get_option('documentation')
doc_deps = [find_program('gtkdoc-scan', required: doc_option), find_program('xsltproc', required: doc_option)]
subdir('docs', if_found: doc_deps)
