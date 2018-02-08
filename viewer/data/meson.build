po_dir = join_paths (meson.source_root (), 'po')

appdata_conf = configuration_data()
appdata_conf.set ('ARAVIS_API_VERSION', aravis_api_version)

configure_file (input: files('arv-viewer.appdata.xml.in'),
		output: 'arv-viewer-@0@.appdata.xml'.format (aravis_api_version),
		configuration: appdata_conf,
		install_dir: aravis_desktop_dir)

desktop_conf = configuration_data()

desktop = i18n.merge_file ('desktop',
			   input: configure_file (input: files('arv-viewer.desktop.in.in'),
						  output: 'arv-viewer.desktop.in',
						  configuration: desktop_conf),
			   output: 'arv-viewer-@0@.desktop'.format (aravis_api_version),
			   install: true,
			   install_dir: aravis_desktop_dir,
			   po_dir: po_dir,
			   type: 'desktop')

