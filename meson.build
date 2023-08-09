project ('aravis', 'c', 'cpp', version: '0.8.30', meson_version: '>=0.57.0')

gnome = import('gnome')
pkg = import ('pkgconfig')

aravis_version = meson.project_version ()
aravis_api_version = '0.8'

versions = aravis_version.split ('.')
aravis_major_version = versions[0].to_int ()
aravis_minor_version = versions[1].to_int ()
aravis_micro_version = versions[2].to_int ()

aravis_data_dir = get_option ('datadir') / 'aravis-@0@'.format (aravis_api_version)

cc = meson.get_compiler ('c')
cxx = meson.get_compiler ('cpp')

if cc.get_id()=='gcc' or cc.get_id()=='clang'
  warning_c_args = [
    '-Wduplicated-branches',
    '-Wimplicit-fallthrough',
    '-Wmisleading-indentation',
    '-Wstrict-prototypes',
    '-Wunused',
    '-Wunused-variable',
    '-Wdeclaration-after-statement',
    '-Wformat=2',
    '-Wimplicit-function-declaration',
    '-Winit-self',
    '-Wmissing-include-dirs',
    '-Wmissing-prototypes',
    '-Wpointer-arith',
    '-Wformat-nonliteral',
    '-Wenum-conversion',
    '-Wmaybe-uninitialized'
    ]

  add_project_arguments(cc.get_supported_arguments(warning_c_args), language: 'c')
elif cc.get_id()=='msvc'
  warning_c_args = [
    '/wd4244', # Disable int-float implicit casting warnings
    '/wd4090', # Disable const qualifiers warnings
    '/utf-8'
  ]

  add_project_arguments(cc.get_supported_arguments(warning_c_args), language: 'c')
endif

if cc.get_id() == 'msvc'
  cc_export_define = '__declspec(dllexport) extern'
elif cc.has_argument('-fvisibility=hidden')
  add_project_arguments('-fvisibility=hidden', language: 'c')
  cc_export_define = 'extern __attribute__ ((visibility ("default")))'
else
  cc_export_define = 'extern'
endif

glib_dep = dependency ('glib-2.0', version: '>=2.52', required: true)
gobject_dep = dependency ('gobject-2.0', required: true)
gio_dep = dependency ('gio-2.0', required: true)
xml2_dep = dependency ('libxml-2.0', required: true)
libz_dep = dependency ('zlib', required: true)
usb_dep = dependency ('libusb-1.0', required: get_option ('usb'))

aravis_public_dependencies = [glib_dep, gobject_dep, gio_dep]
aravis_dependencies = [aravis_public_dependencies, xml2_dep, libz_dep]
if cc.get_id()!='msvc'
	libm = cc.find_library ('m', required: true)
	aravis_dependencies += libm
endif
if usb_dep.found()
  aravis_dependencies += usb_dep
endif

if host_machine.system()=='windows'
	aravis_dependencies += [
		cc.find_library('ws2_32', required: true),  # socket functions
		cc.find_library('iphlpapi', required: true) # GetAdaptersAddresses
	]
endif

packet_socket_option = get_option('packet-socket')
if host_machine.system()=='linux'
	has_if_packet = cc.has_header ('linux' /'if_packet.h')
	if packet_socket_option.enabled()
		if not has_if_packet
			error ('missing header for packet-socket support')
		endif
		packet_socket_enabled = true
	else
		packet_socket_enabled = has_if_packet and packet_socket_option.auto()
	endif
else # not Linux
	if packet_socket_option.enabled()
		warning('packet-socket option ignored on non-Linux')
	endif
	packet_socket_enabled = false
endif

subdir ('src')
subdir ('tests')

viewer_enabled = false
viewer_option = get_option ('viewer')
viewer_deps = aravis_dependencies + [dependency ('gtk+-3.0', version: '>=3.12', required: viewer_option),
				     dependency ('gstreamer-base-1.0', required: viewer_option),
				     dependency ('gstreamer-app-1.0', required: viewer_option),
				     dependency ('gstreamer-video-1.0', required: viewer_option)]
subdir ('po', if_found: viewer_deps)
subdir ('viewer', if_found: viewer_deps)

gst_enabled = false
gst_option = get_option ('gst-plugin')
gst_deps = aravis_dependencies + [dependency ('gstreamer-base-1.0', required: gst_option),
                                  dependency ('gstreamer-app-1.0', required: gst_option)]
subdir('gst', if_found: gst_deps)

doc_deps = dependency ('gi-docgen', version:'>= 2021.1', fallback: ['gi-docgen', 'dummy_dep'], required:get_option('documentation'))
subdir('docs', if_found: doc_deps)

meson.override_dependency ('aravis-' + aravis_api_version, aravis_library_dependencies)

summary (
  {
  'prefix': get_option('prefix'),
  'bindir': get_option('bindir'),
  'libdir': get_option('libdir'),
  'datadir': get_option('datadir'),
  },
  section: 'Directories'
)

summary (
  {
  'Viewer': viewer_enabled,
  'GStreamer plugin': gst_enabled,
  'USB support': usb_dep.found(),
  },
  section: 'Options'
)
