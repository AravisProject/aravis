subdir ('xml')

doc_ignore_headers = library_private_headers + [
	'arv.h',
	'arvconfig.h',
	'arvenumtypes.h',
	'arvenumtypesprivate.h'
]

if not usb_dep.found()
	doc_ignore_headers += [
		'arvuvinterface.h',
		'arvuvdevice.h',
		'arvuvstream.h',
		'arvuvspprivate.h',
		'arvuvcpprivate.h',
		'arvuvdeviceprivate.h',
		'arvuvinterfaceprivate.h',
		'arvuvstreamprivate.h'
	]
endif

gnome.gtkdoc ('aravis',
	      main_xml: 'aravis-docs.xml',
	      src_dir: [join_paths (meson.source_root (), 'src'),
	                join_paths (meson.build_root (), 'src')],
	      gobject_typesfile: 'aravis.types',
	      dependencies: [aravis_library_dependencies],
	      scan_args: ['--ignore-headers=' + ' '.join (doc_ignore_headers), '--rebuild-types', '--ignore-decorator=ARV_API'],
	      content_files: ['aravis-overview.xml', 'aravis-gv.xml'] + doc_examples,
	      install_dir: 'aravis-@0@'.format (aravis_api_version),
	      install: true)
