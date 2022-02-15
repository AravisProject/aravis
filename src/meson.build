library_sources = [
	'arvenums.c',
	'arvdebug.c',
	'arvsystem.c',
	'arvevaluator.c',
	'arvdomnode.c',
	'arvdomnodechildlist.c',
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
        'arvgcenums.c',
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
	'arvgcintregnode.c',
	'arvgcstringnode.c',
	'arvgcmaskedintregnode.c',
	'arvgcfloatregnode.c',
	'arvgcstringregnode.c',
	'arvgcstructregnode.c',
	'arvgcstructentrynode.c',
	'arvgccommand.c',
	'arvgcswissknife.c',
	'arvgcswissknifenode.c',
	'arvgcintswissknifenode.c',
	'arvgcconverter.c',
	'arvgcconverternode.c',
	'arvgcintconverternode.c',
	'arvgcport.c',
	'arvgcregister.c',
	'arvgcinteger.c',
	'arvgcstring.c',
	'arvgcselector.c',
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
	'arvnetwork.c',
	'arvzip.c',
	'arvstr.c',
	'arvgvcp.c',
	'arvgvsp.c',
	'arvwakeup.c'
]

library_headers = [
	'arv.h',
	'arvtypes.h',

	'arvbuffer.h',
	'arvcamera.h',
	'arvchunkparser.h',
	'arvdebug.h',
	'arvdevice.h',

	'arvdomcharacterdata.h',
	'arvdomdocumentfragment.h',
	'arvdomdocument.h',
	'arvdomelement.h',
	'arvdomimplementation.h',
	'arvdomnamednodemap.h',
	'arvdomnode.h',
	'arvdomnodechildlist.h',
	'arvdomnodelist.h',
	'arvdomparser.h',
	'arvdomtext.h',

	'arvenums.h',
	'arvevaluator.h',

	'arvfakecamera.h',
	'arvfakedevice.h',
	'arvfakeinterface.h',
	'arvfakestream.h',

	'arvgcboolean.h',
	'arvgccategory.h',
	'arvgccommand.h',
	'arvgcconverter.h',
	'arvgcconverternode.h',
	'arvgcenumentry.h',
	'arvgcenumeration.h',
	'arvgcenums.h',
	'arvgcfeaturenode.h',
	'arvgcfloat.h',
	'arvgcfloatnode.h',
	'arvgcgroupnode.h',
	'arvgc.h',
	'arvgcindexnode.h',
	'arvgcintconverternode.h',
	'arvgcinteger.h',
	'arvgcintegernode.h',
	'arvgcinvalidatornode.h',
	'arvgcnode.h',
	'arvgcport.h',
	'arvgcpropertynode.h',
	'arvgcregisterdescriptionnode.h',
	'arvgcregister.h',
	'arvgcregisternode.h',
	'arvgcintregnode.h',
	'arvgcstringnode.h',
	'arvgcmaskedintregnode.h',
	'arvgcfloatregnode.h',
	'arvgcstringregnode.h',
	'arvgcstructregnode.h',
	'arvgcstring.h',
	'arvgcselector.h',
	'arvgcstructentrynode.h',
	'arvgcswissknife.h',
	'arvgcswissknifenode.h',
	'arvgcintswissknifenode.h',
	'arvgcvalueindexednode.h',

	'arvgvdevice.h',
	'arvgvfakecamera.h',
	'arvgvinterface.h',
	'arvgvstream.h',

	'arvinterface.h',
	'arvsystem.h',
	'arvrealtime.h',
	'arvstream.h',
	'arvxmlschema.h'
]

library_private_headers = [
	'arvbufferprivate.h',
	'arvchunkparserprivate.h',
	'arvdebugprivate.h',
	'arvdeviceprivate.h',
	'arvfakedeviceprivate.h',
	'arvfakeinterfaceprivate.h',
	'arvfakestreamprivate.h',
	'arvgcprivate.h',
	'arvgcconverterprivate.h',
	'arvgcdefaultsprivate.h',
	'arvgcfeaturenodeprivate.h',
	'arvgcregisternodeprivate.h',
	'arvgcswissknifeprivate.h',
	'arvgvcpprivate.h',
	'arvgvdeviceprivate.h',
	'arvgvinterfaceprivate.h',
	'arvgvspprivate.h',
	'arvgvstreamprivate.h',
	'arvinterfaceprivate.h',
	'arvmiscprivate.h',
	'arvnetworkprivate.h',
	'arvrealtimeprivate.h',
	'arvstreamprivate.h',
	'arvwakeupprivate.h'
]

library_no_introspection_headers = [
	'arvmisc.h',
	'arvzip.h',
	'arvstr.h'
]

if usb_dep.found()
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
	library_private_headers += [
		'arvuvcpprivate.h',
		'arvuvdeviceprivate.h',
		'arvuvinterfaceprivate.h',
		'arvuvstreamprivate.h',
		'arvuvspprivate.h'
		]
endif

library_enums = gnome.mkenums_simple ('arvenumtypes', decorator: 'ARV_API', header_prefix: '#include <arvapi.h>',
				      sources: library_headers + library_no_introspection_headers)
library_private_enums = gnome.mkenums_simple ('arvenumtypesprivate', sources: library_private_headers)
library_resources = gnome.compile_resources('arvresources','arvresources.xml')

library_include_dir = get_option ('includedir') / 'aravis-@0@'.format (aravis_api_version)

api_config_data = configuration_data ()
api_config_data.set ('ARV_API', cc_export_define)
configure_file (input: 'arvapi.h.in', output: 'arvapi.h',
		configuration: api_config_data, install_dir: library_include_dir)

features_library_config_data = configuration_data ()
features_library_config_data.set10 ('ARAVIS_HAS_USB', usb_dep.found())
features_library_config_data.set10 ('ARAVIS_HAS_PACKET_SOCKET', packet_socket_enabled)
features_library_config_data.set10 ('ARAVIS_HAS_FAST_HEARTBEAT', get_option ('fast-heartbeat'))
configure_file (input: 'arvfeatures.h.in', output: 'arvfeatures.h',
		configuration: features_library_config_data, install_dir: library_include_dir)

params_library_config_data = configuration_data ()
params_library_config_data.set ('ARV_GV_STREAM_NUM_BUFFERS', get_option ('gv-n-buffers'))
configure_file (input: 'arvparamsprivate.h.in', output: 'arvparamsprivate.h',
		configuration: params_library_config_data)

version_config_data = configuration_data ()
version_config_data.set ('ARAVIS_VERSION', aravis_version)
version_config_data.set ('ARAVIS_API_VERSION', aravis_api_version)
version_config_data.set ('ARAVIS_MAJOR_VERSION', aravis_major_version)
version_config_data.set ('ARAVIS_MINOR_VERSION', aravis_minor_version)
version_config_data.set ('ARAVIS_MICRO_VERSION', aravis_micro_version)
configure_file (input: 'arvversion.h.in', output: 'arvversion.h',
		configuration: version_config_data, install_dir: library_include_dir)

library_inc = include_directories ('.')

install_headers (library_headers + library_no_introspection_headers, install_dir: library_include_dir)

library_c_args = [
	'-DARAVIS_COMPILATION'
	]

aravis_library = library ('aravis-@0@'.format (aravis_api_version),
	library_sources, library_headers,
	library_no_introspection_sources, library_no_introspection_headers, library_private_headers,
	library_enums, library_private_enums,
	library_resources,
	version: aravis_version,
	soversion: '0',
	dependencies: aravis_dependencies,
	c_args: library_c_args,
	install: true)

aravis_library_dependencies = declare_dependency (dependencies: aravis_dependencies,
						  link_with: aravis_library, include_directories: library_inc)

pkg.generate (aravis_library,
	      filebase: 'aravis-@0@'.format (aravis_api_version),
	      version: aravis_version,
	      subdirs: 'aravis-@0@'.format (aravis_api_version),
	      name: 'Aravis',
	      description: 'Camera control and image acquisition library',
	      requires: aravis_public_dependencies)

arv_test_resources = gnome.compile_resources('arvtestresources','arvtestresources.xml')

progs = [
  [ 'arv-tool', 		'arvtool.c'],
  [ 'arv-test', 		['arvtest.c'] + arv_test_resources],
  [ 'arv-camera-test',		'arvcameratest.c'],
  [ 'arv-fake-gv-camera', 	'arvfakegvcamera.c'],
]

foreach p: progs
	executable ((p[0] + '-@0@').format (aravis_api_version),
		p[1],
		link_with: aravis_library,
		c_args: library_c_args,
		dependencies: aravis_dependencies,
		install: true)
endforeach

introspection_option = get_option('introspection')
introspection_enabled = false
if introspection_option.auto() or introspection_option.enabled()
  gir_scanner = find_program('g-ir-scanner', required: introspection_option)
  gi_dep = dependency('gobject-introspection-1.0', required: introspection_option)

  if gir_scanner.found() and gi_dep.found()
    aravis_gir = gnome.generate_gir(aravis_library,
      sources : [library_sources, library_headers, library_enums],
      nsversion : aravis_api_version,
      namespace : 'Aravis',
      symbol_prefix : 'arv_',
      identifier_prefix : 'Arv',
      header: 'arv.h',
      export_packages : 'aravis-@0@'.format(aravis_api_version),
      includes : ['GObject-2.0', 'Gio-2.0'],
      extra_args: '-DARAVIS_COMPILATION',
      install : true)
    introspection_enabled = true
  endif
endif
