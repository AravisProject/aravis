subdir ('xml')

doc_ignore_headers = [
	'arv.h',
	'arvconfig.h',
	'arvenumtypes.h',
	'arvenumtypesprivate.h',

	'arvbufferprivate.h',
	'arvchunkparserprivate.h',
	'arvdeviceprivate.h',
	'arvfakedeviceprivate.h',
	'arvfakeinterfaceprivate.h',
	'arvfakestreamprivate.h',
	'arvgcconverterprivate.h',
	'arvgcfeaturenodeprivate.h',
	'arvgcregisternodeprivate.h',
	'arvgcswissknifeprivate.h',
	'arvgvcpprivate.h',
	'arvgvstreamprivate.h',
	'arvgvdeviceprivate.h',
	'arvgvinterfaceprivate.h',
	'arvgvspprivate.h',
	'arvinterfaceprivate.h',
	'arvmiscprivate.h',
	'arvrealtimeprivate.h',
	'arvstreamprivate.h',
	'arvwakeupprivate.h',

	'arvuvdeviceprivate.h',
	'arvuvinterfaceprivate.h',
	'arvuvstreamprivate.h',
	'arvuvspprivate.h',
	'arvuvcpprivate.h'
]

if not enable_usb
	doc_ignore_headers += [
		'arvuvinterface.h',
		'arvuvdevice.h',
		'arvuvstream.h'
	]
endif

gnome.gtkdoc ('aravis',
	      main_xml: 'aravis-docs.xml',
	      src_dir: [join_paths (meson.source_root (), 'src'),
	                join_paths (meson.build_root (), 'src')],
	      gobject_typesfile: 'aravis.types',
	      dependencies: [aravis_library_dependencies],
	      scan_args: ['--ignore-headers=' + ' '.join (doc_ignore_headers), '--rebuild-types'],
	      content_files: ['aravis-overview.xml',
	      		      'aravis-gv.xml',
			      '../../../tests/arvexample.c',
			      '../../../tests/arvchunkparsertest.c'],
	      install_dir: 'aravis-@0@'.format (aravis_api_version),
	      install: true)
