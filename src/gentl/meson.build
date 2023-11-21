gentl_library = shared_module ('aravis-@0@'.format (aravis_version),
	name_prefix: '',
	name_suffix: 'cti',
	install: true,
	sources: ['dev.c', 'ds.c', 'event.c', 'gc.c', 'if.c', 'tl.c', 'GenTL_v1_6.h', 'private.h', 'private.c', 'private-buf.c'],
	link_with: aravis_library,
	include_directories: library_inc,
	c_args: library_c_args,
	dependencies: aravis_dependencies
)
