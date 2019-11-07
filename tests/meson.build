tests = [
	['evaluator',	[]],
	['buffer', 	[]],
	['misc', 	[]],
	['fake',	['-DGENICAM_FILENAME="@0@/src/arv-fake-camera.xml"'.format (meson.source_root ())]],
	['fakegv',	['-DGENICAM_FILENAME="@0@/src/arv-fake-camera.xml"'.format (meson.source_root ())]],
	['genicam',	['-DGENICAM_FILENAME="@0@/tests/data/genicam.xml"'.format (meson.source_root ())]]
]

foreach t: tests
  exe = executable (t[0], '@0@.c'.format (t[0]),
    c_args: [t[1]],
    link_with: aravis_library,
    dependencies: aravis_dependencies,
    include_directories: [library_inc])
  test (t[0], exe)
endforeach

python_tests = [
	['fake.py',		[]],
	['exception.py',	[]]
]

if get_option ('introspection')
	pymod = import ('python')

	py = pymod.find_installation ('python3', required: false)

	if py.found()
		environment = [
			'GI_TYPELIB_PATH=' + join_paths (meson.build_root() , 'src'),
			'LD_LIBRARY_PATH=' + join_paths (meson.build_root() , 'src')
		]

		foreach t: python_tests
			test (t[0], py, args: files (t[0]), env: environment)
		endforeach
	endif
endif

examples = [
	['arv-device-test',		'arvdevicetest.c'],
	['arv-genicam-test',		'arvgenicamtest.c'],
	['arv-evaluator-test',	  	'arvevaluatortest.c'],
	['arv-zip-test',		'arvziptest.c'],
	['arv-camera-test',		'arvcameratest.c'],
	['arv-chunk-parser-test',	'arvchunkparsertest.c'],
	['arv-heartbeat-test',		'arvheartbeattest.c'],
	['arv-acquisition-test',	'arvacquisitiontest.c'],
	['arv-example',			'arvexample.c'],
	['arv-auto-packet-size-test',	'arvautopacketsizetest.c'],
	['arv-device-scan-test',	'arvdevicescantest.c'],
	['arv-roi-test',		'arvroitest.c'],
	['time-test',			'timetest.c'],
	['realtime-test',		'realtimetest.c'],
	['load-http-test',		'loadhttptest.c'],
	['cpp-test',			'cpp.cc']
]

foreach example: examples
  exe = executable (example[0], example[1],
    link_with: aravis_library,
    dependencies: aravis_dependencies,
    include_directories: [library_inc])
endforeach
