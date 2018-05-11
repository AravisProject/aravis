gst_sources = [
	'gstaravis.c'
]

gst_headers = [
	'gstaravis.h'
]

gst_c_args = [
	'-DPACKAGE_LOCALE_DIR=@0@'.format(get_option ('prefix')),
	'-DG_LOG_DOMAIN="Aravis"'
]

gst_plugin = shared_library ('gstplugin',
			     gst_sources, gst_headers,
			     link_with: aravis_library,
			     c_args: gst_c_args,
			     include_directories: [configuration_inc, library_inc],
			     dependencies: gst_plugin_dependencies)
