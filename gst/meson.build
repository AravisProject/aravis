gst_plugin_dir = join_paths (get_option ('libdir'), 'gstreamer-1.0')

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

gst_plugin_filename = 'gstaravis.@0@'.format (aravis_api_version)

gst_plugin = shared_library (gst_plugin_filename,
			     gst_sources, gst_headers,
			     name_suffix: [],
			     link_with: aravis_library,
			     c_args: gst_c_args,
			     include_directories: [library_inc],
			     dependencies: gst_deps,
			     install: true,
			     install_dir: gst_plugin_dir)

gst_script_config_data = configuration_data ()
gst_script_config_data.set ('GST_PLUGIN_FILENAME', gst_plugin.full_path ())
configure_file (input: 'gst-aravis-launch.in', output: 'gst-aravis-launch',
		configuration: gst_script_config_data)
configure_file (input: 'gst-aravis-inspect.in', output: 'gst-aravis-inspect',
		configuration: gst_script_config_data)
