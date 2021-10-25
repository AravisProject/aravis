po_dir = join_paths (meson.source_root (), 'po')

appdata_conf = configuration_data()
appdata_conf.set ('ARAVIS_API_VERSION', aravis_api_version)

configure_file (input: files('arv-viewer.appdata.xml.in'),
		output: 'arv-viewer-@0@.appdata.xml'.format (aravis_api_version),
		configuration: appdata_conf,
		install_dir: aravis_app_data_dir)

desktop = i18n.merge_file (input: configure_file (input: files('arv-viewer.desktop.in.in'),
						  output: 'arv-viewer.desktop.in',
						  configuration: appdata_conf),
			   output: 'arv-viewer-@0@.desktop'.format (aravis_api_version),
			   install: true,
			   install_dir: aravis_desktop_dir,
			   po_dir: po_dir,
			   type: 'desktop')
