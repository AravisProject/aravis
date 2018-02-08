viewer_sources = [
	'main.c',
	'arvviewer.c'
]

viewer_headers = [
	'arvviewertypes.h',
	'arvviewer.h'
]

viewer_c_args = [
	'-I../src',
	'-DPACKAGE_LOCALE_DIR="@0@"'.format (join_paths (get_option ('prefix'), 'locale')),
	'-DARAVIS_DATA_DIR="@0@"'.format (join_paths (get_option ('prefix'), aravis_data_dir)),
	'-DARAVIS_API_VERSION="@0@"'.format (aravis_api_version),
	'-DARAVIS_ICON_DIR="@0@"'.format (join_paths (get_option ('prefix'), aravis_data_dir, 'icons'))
]

executable ('arv-viewer-@0@'.format (aravis_api_version),
	    viewer_sources, viewer_headers,
	    link_with: aravis_library,
	    c_args: viewer_c_args,
	    include_directories: [configuration_inc, library_inc],
	    dependencies: aravis_viewer_dependencies,
	    install: true)

install_data ('arv-viewer.ui', install_dir: aravis_data_dir)

install_subdir (join_paths ('icons', 'gnome'),
		install_dir: aravis_icon_dir,
		strip_directory: true)

subdir ('data')
