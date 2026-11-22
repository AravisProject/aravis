gv_mkfile = custom_target('gv_makefile', output:'Makefile', command:['touch', '@OUTPUT@'])

gv_sources = files([
  'arvgvkmod.c'
])

custom_target ('gv_kmod',input: gv_sources, output: 'aravis_gv.ko', command:[
  'make', '-j4', '-C', kernel_build_dir,
  'M='+join_paths(meson.current_build_dir()),
  'src='+join_paths(meson.current_source_dir()),
  'MODULE_CFLAGS=-I'+meson.current_source_dir(),
  'modules'], depends:gv_mkfile, build_by_default:true)

