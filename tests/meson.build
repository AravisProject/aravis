
if get_option('tests')

  add_test_setup (
    'valgrind',
    exe_wrapper:[
      'valgrind',
      '--tool=memcheck',
      '--num-callers=50',
      '--leak-check=yes',
      '--suppressions=' + meson.project_source_root() / 'tests' / 'aravis.supp',
      '--error-exitcode=1'
    ],
    env:['DEBUGINFOD_URLS=', 'ARV_TEST_IGNORE_BUFFER='],
    exclude_suites:['python', 'bench'],
    timeout_multiplier:5
  )

  # The stream bench is a measurement, not a test: run it with 'meson test --suite bench' or directly.
  add_test_setup ('default', exclude_suites: ['bench'], is_default: true)

	tests = [
		['evaluator',	['main'],    []],
		['buffer',	['main'],    []],
		['misc',	['main'],    []],
		['dom',		['main'],
                ['-DGENICAM_FILENAME="@0@/tests/data/genicam.xml"'.format (meson.project_source_root ())]],
		['genicam',	['main'],
                ['-DGENICAM_FILENAME="@0@/tests/data/genicam.xml"'.format (meson.project_source_root ())]],
		['fake',	['main'],
                ['-DGENICAM_FILENAME="@0@/src/arv-fake-camera.xml"'.format (meson.project_source_root ())]]
	]

	foreach t: tests
		exe = executable (t[0], '@0@.c'.format (t[0]),
				  c_args: [t[2]],
				  link_with: aravis_library,
				  dependencies: aravis_dependencies,
				  include_directories: [library_inc])
		test (t[0], exe, suite: t[1], timeout: 60)
	endforeach

	# The GigE Vision fake camera tests, once per receive path: the default (on Apple the recvmsg_x loop, fast
	# fail and a connected stream socket), the standard loop, upstream frame logic, and an unconnected socket.
	# Every instance binds 127.0.0.1:3956, so none may run in parallel with another.
	fakegv = executable ('fakegv', 'fakegv.c',
			     c_args: ['-DGENICAM_FILENAME="@0@/src/arv-fake-camera.xml"'.format (meson.project_source_root ())],
			     link_with: aravis_library,
			     dependencies: aravis_dependencies,
			     include_directories: [library_inc])
	foreach t: [['fakegv', []],
		    ['fakegv-rx-batch-0', ['ARV_GV_STREAM_RX_BATCH=0']],
		    ['fakegv-no-fast-fail', ['ARV_GV_STREAM_FAST_FAIL=0']],
		    ['fakegv-no-connect', ['ARV_GV_STREAM_CONNECT=0']]]
		test (t[0], fakegv, suite: 'network', env: t[1], timeout: 180, is_parallel: false)
	endforeach

	if get_option ('gentl-producer')
		gentl_buffer_test = executable (
			'gentl-buffer',
			['gentl-buffer.c', '../src/gentl/private-buf.c'],
			c_args: library_c_args,
			link_with: aravis_library,
			dependencies: aravis_dependencies,
			include_directories: [library_inc, include_directories ('../src/gentl')]
		)
		test ('gentl-buffer', gentl_buffer_test, suite: 'main')

		gentl_module_test = executable (
			'gentl-module',
			'gentl-module.c',
			c_args: library_c_args,
			link_with: aravis_library,
			dependencies: aravis_dependencies,
			include_directories: [library_inc]
		)
		test (
			'gentl-module',
			gentl_module_test,
			suite: 'main',
			env: [
				'ARV_PRODUCER_PATH=' + gentl_producer_library.full_path (),
				'FAKE_GENICAM_PATH=' + meson.project_source_root() / 'src' / 'arv-fake-camera.xml'
			],
			depends: gentl_producer_library
		)
	endif

        py_script_config_data = configuration_data ()
        py_script_config_data.set ('GI_TYPELIB_PATH', meson.project_build_root() / 'src')
        py_script_config_data.set ('LD_LIBRARY_PATH', meson.project_build_root() / 'src')
        py_script_config_data.set ('GST_PLUGIN_PATH', meson.project_build_root() / 'gst')
        py_script_config_data.set ('FAKE_GENICAM_PATH', meson.project_source_root() / 'src' / 'arv-fake-camera.xml')
        py_script_config_data.set ('ARV_PRODUCER_PATH', get_option('gentl-producer') ? gentl_producer_library.full_path() : '')

        configure_file (input: 'pylaunch.in', output: 'pylaunch', configuration: py_script_config_data)
        configure_file (input: 'pylaunch-dbg.in', output: 'pylaunch-dbg', configuration: py_script_config_data)
        configure_file (input: 'jslaunch.in', output: 'jslaunch', configuration: py_script_config_data)
        configure_file (input: 'jslaunch-dbg.in', output: 'jslaunch-dbg', configuration: py_script_config_data)

	if host_machine.system() != 'windows'
		# POSIX only: getrusage, getloadavg and SIGINT to the child sender
		bench = executable ('arv-gv-stream-bench', 'arvgvstreambench.c',
				    c_args: ['-DARV_FAKE_GV_CAMERA_PATH="@0@"'.format (meson.project_build_root () / 'src' /
										    'arv-fake-gv-camera-@0@'.format (aravis_api_version))],
				    link_with: aravis_library,
				    dependencies: aravis_dependencies,
				    include_directories: [library_inc])
		test ('gvstreambench', bench, suite: 'bench', args: ['--duration', '3', '--frame-rate', '50'],
		      timeout: 300, is_parallel: false)
	endif

	if introspection_enabled
		pymod = import ('python')

		py = pymod.find_installation ('python3', required: false)

		if py.found()
			python_tests = [
			  ['fake.py',		[]],
			  ['exception.py',	[]],
			  ['device.py',	        []],
			]

                        if gst_enabled
                          python_tests += [['gst.py', []]]
                        endif

                        message(python_tests)

			environment = [
				'GI_TYPELIB_PATH=' + meson.project_build_root() / 'src',
				'LD_LIBRARY_PATH=' + meson.project_build_root() / 'src',
				'FAKE_GENICAM_PATH=' + meson.project_source_root() / 'src' / 'arv-fake-camera.xml',
                                'GST_PLUGIN_PATH=' + meson.project_build_root() / 'gst'
			]

			foreach t: python_tests
				test (t[0], py, suite: 'python', args: files (t[0]), env: environment)
			endforeach
		endif

	endif

	examples = [
		['arv-network-test',		'arvnetworktest.c'],
		['arv-device-test',		'arvdevicetest.c'],
		['arv-genicam-test',		'arvgenicamtest.c'],
		['arv-evaluator-test',		'arvevaluatortest.c'],
		['arv-zip-test',		'arvziptest.c'],
		['arv-chunk-parser-test',	'arvchunkparsertest.c'],
		['arv-heartbeat-test',		'arvheartbeattest.c'],
		['arv-acquisition-test',	'arvacquisitiontest.c'],
		['arv-example',			'arvexample.c'],
		['arv-auto-packet-size-test',	'arvautopacketsizetest.c'],
		['arv-device-scan-test',	'arvdevicescantest.c'],
		['arv-roi-test',		'arvroitest.c'],
		['arv-multi-uv-test',		'arvmultiuvtest.c'],
		['time-test',			'timetest.c'],
		['load-http-test',		'loadhttptest.c'],
		['cpp-test',			'cpp.cc'],
	]

        if get_option ('v4l2').enabled()
          examples += [
		['v4l2-test',			'v4l2test.c'],
            ]
        endif

	if host_machine.system()=='linux'
		examples+=[['realtime-test','realtimetest.c']] # uses Linux RT API unavailable on other platforms
	endif

	foreach example: examples
		executable (example[0], example[1],
			link_with: aravis_library,
			dependencies: aravis_dependencies,
			include_directories: [library_inc])
	endforeach

        if gst_enabled
          exe = executable (
            'gst', 'gst.c',
            link_with: aravis_library,
            dependencies: [aravis_dependencies, gst_deps],
            c_args: [
              '-DGENICAM_FILENAME="@0@/src/arv-fake-camera.xml"'.format (meson.project_source_root ()),
              '-DGST_PLUGIN_PATH="@0@"'.format(meson.project_build_root() / 'gst')
            ],
            include_directories: [library_inc]
          )
          test ('gst', exe, suite: 'main', timeout: 60)
        endif

endif
