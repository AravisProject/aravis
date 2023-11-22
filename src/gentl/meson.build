gentl_producer_sources = [
  'dev.c',
  'ds.c',
  'event.c',
  'gc.c',
  'if.c',
  'tl.c',
  'private.c',
  'private-buf.c',
]

gentl_producer_headers = [
  'GenTL_v1_6.h',
  'private.h'
]

gentl_producer_library = shared_module (
  'ProducerAravis-@0@'.format (aravis_api_version),
  name_prefix: '',
  name_suffix: 'cti',
  install: true,
  sources: gentl_producer_sources,
  link_with: aravis_library,
  include_directories: library_inc,
  c_args: library_c_args,
  dependencies: aravis_library_dependencies
)
