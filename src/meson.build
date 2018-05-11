library_sources = [
	'arvenums.c',
	'arvdebug.c',
	'arvsystem.c',
	'arvevaluator.c',
	'arvdomnode.c',
	'arvdomnodelist.c',
	'arvdomnamednodemap.c',
	'arvdomdocument.c',
	'arvdomdocumentfragment.c',
	'arvdomelement.c',
	'arvdomcharacterdata.c',
	'arvdomtext.c',
	'arvdomparser.c',
	'arvdomimplementation.c',
	'arvcamera.c',
	'arvgc.c',
	'arvgcnode.c',
	'arvgcpropertynode.c',
	'arvgcindexnode.c',
	'arvgcvalueindexednode.c',
	'arvgcinvalidatornode.c',
	'arvgcfeaturenode.c',
	'arvgcregisterdescriptionnode.c',
	'arvgcgroupnode.c',
	'arvgccategory.c',
	'arvgcboolean.c',
	'arvgcenumeration.c',
	'arvgcenumentry.c',
	'arvgcintegernode.c',
	'arvgcfloatnode.c',
	'arvgcregisternode.c',
	'arvgcstructentrynode.c',
	'arvgccommand.c',
	'arvgcswissknife.c',
	'arvgcconverter.c',
	'arvgcport.c',
	'arvgcregister.c',
	'arvgcinteger.c',
	'arvgcstring.c',
	'arvgcfloat.c',
	'arvinterface.c',
	'arvdevice.c',
	'arvstream.c',
	'arvbuffer.c',
	'arvchunkparser.c',
	'arvgvinterface.c',
	'arvgvdevice.c',
	'arvgvstream.c',
	'arvfakeinterface.c',
	'arvfakedevice.c',
	'arvfakestream.c',
	'arvfakecamera.c',
	'arvgvfakecamera.c',
	'arvrealtime.c',
	'arvxmlschema.c'
]

library_no_introspection_sources = [
	'arvmisc.c',
	'arvzip.c',
	'arvstr.c',
	'arvgvcp.c',
	'arvgvsp.c'
]

library_headers = [
	'arv.h',
	'arvtypes.h',
	'arvenums.h',
	'arvdebug.h',
	'arvsystem.h',
	'arvevaluator.h',
	'arvdomnode.h',
	'arvdomnodelist.h',
	'arvdomnamednodemap.h',
	'arvdomdocument.h',
	'arvdomdocumentfragment.h',
	'arvdomelement.h',
	'arvdomcharacterdata.h',
	'arvdomtext.h',
	'arvdomparser.h',
	'arvdomimplementation.h',
	'arvcamera.h',
	'arvgc.h',
	'arvgcnode.h',
	'arvgcpropertynode.h',
	'arvgcindexnode.h',
	'arvgcvalueindexednode.h',
	'arvgcinvalidatornode.h',
	'arvgcfeaturenode.h',
	'arvgcregisterdescriptionnode.h',
	'arvgcgroupnode.h',
	'arvgccategory.h',
	'arvgcboolean.h',
	'arvgcenumeration.h',
	'arvgcenumentry.h',
	'arvgcintegernode.h',
	'arvgcfloatnode.h',
	'arvgcregisternode.h',
	'arvgcstructentrynode.h',
	'arvgccommand.h',
	'arvgcswissknife.h',
	'arvgcconverter.h',
	'arvgcport.h',
	'arvgcregister.h',
	'arvgcinteger.h',
	'arvgcstring.h',
	'arvgcfloat.h',
	'arvinterface.h',
	'arvdevice.h',
	'arvstream.h',
	'arvbuffer.h',
	'arvchunkparser.h',
	'arvgvinterface.h',
	'arvgvdevice.h',
	'arvgvstream.h',
	'arvfakeinterface.h',
	'arvfakedevice.h',
	'arvfakestream.h',
	'arvfakecamera.h',
	'arvgvfakecamera.h',
	'arvrealtime.h',
	'arvxmlschema.h'
]

library_no_introspection_headers = [
	'arvmisc.h',
	'arvzip.h',
	'arvstr.h',
	'arvgvcp.h',
	'arvgvsp.h'
]

if get_option ('usb')
	library_sources += [
		'arvuvinterface.c',
		'arvuvdevice.c',
		'arvuvstream.c'
	]
	library_no_introspection_sources += [
		'arvuvcp.c',
		'arvuvsp.c'
	]
	library_headers += [
		'arvuvinterface.h',
		'arvuvdevice.h',
		'arvuvstream.h'
	]
	library_no_introspection_headers += [
		'arvuvcp.h',
		'arvuvsp.h'
		]
endif

library_enums = gnome.mkenums_simple ('arvenumtypes', sources: library_headers + library_no_introspection_headers)

library_include_dir = join_paths (get_option ('includedir'), 'aravis-@0@'.format (aravis_api_version))

library_config_data = configuration_data ()
library_config_data.set10 ('ARAVIS_HAS_USB', get_option ('usb'))
library_config_data.set10 ('ARAVIS_HAS_PACKET_SOCKET', get_option ('packet-socket'))
configure_file (input: 'arvfeatures.h.in', output: 'arvfeatures.h',
		configuration: library_config_data, install_dir: library_include_dir)

version_config_data = configuration_data ()
version_config_data.set ('ARAVIS_MAJOR_VERSION', aravis_major_version)
version_config_data.set ('ARAVIS_MINOR_VERSION', aravis_minor_version)
version_config_data.set ('ARAVIS_MICRO_VERSION', aravis_micro_version)
configure_file (input: 'arvversion.h.in', output: 'arvversion.h',
		configuration: version_config_data, install_dir: library_include_dir)

library_inc = include_directories ('.')

install_headers (library_headers, install_dir: library_include_dir)

install_data ('arv-fake-camera.xml', install_dir: aravis_data_dir)

library_c_args = [
	'-DARAVIS_COMPILATION',
         '-DARAVIS_DATA_DIR="@0@"'.format (join_paths(get_option ('prefix'), aravis_data_dir)),
	 '-DARAVIS_API_VERSION="@0@"'.format (aravis_api_version)
	 ]

aravis_library = library ('aravis-@0@'.format (aravis_api_version),
	library_sources, library_headers,
	library_no_introspection_sources, library_no_introspection_headers,
	library_enums,
	version: aravis_version,
	soversion: '0',
	dependencies: aravis_dependencies,
	c_args: library_c_args,
	include_directories: configuration_inc,
	install: true)

aravis_library_dependencies = declare_dependency (dependencies: aravis_dependencies,
						  link_with: aravis_library)

pkg.generate (libraries: [aravis_library, libm],
	      filebase: 'aravis-@0@'.format (aravis_api_version),
	      version: aravis_version,
	      subdirs: 'aravis-@0@'.format (aravis_api_version),
	      name: 'Aravis',
	      description: 'Camera control and image acquisition library',
	      requires: aravis_dependency_str,
	      requires_private: aravis_private_dependency_str)

progs = [[ 'arv-tool', 			'arvtool.c'],
	 [ 'arv-test',			'arvtest.c'],
	 [ 'arv-fake-gv-camera', 	'arvfakegvcamera.c']]

foreach p: progs
	executable ((p[0] + '-@0@').format (aravis_api_version),
		p[1],
		link_with: aravis_library,
		c_args: library_c_args,
		dependencies: aravis_dependencies,
		install: true)
endforeach

if get_option ('introspection')
	gnome.generate_gir(aravis_library,
			   sources : [library_sources, library_headers, library_enums],
			   nsversion : aravis_api_version,
			   namespace : 'Aravis',
			   symbol_prefix : 'arv_',
			   identifier_prefix : 'Arv',
			   export_packages : 'aravis',
			   includes : ['GObject-2.0', 'Gio-2.0'],
			   extra_args: '-DARAVIS_COMPILATION',
			   install : true
			  )
endif
