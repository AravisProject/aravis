toml_conf = configuration_data()
toml_conf.set('version', meson.project_version())

gidocgen = find_program('gi-docgen')

gidocgen_common_args = [
  '--quiet',
  '--no-namespace-dir',
]

docs_dir = get_option('datadir') / 'doc'

expand_content_md_files = [
  'introduction.md',
  'building.md',
  'examples.md',
  'utilities.md',
  'ethernet.md',
  'usb.md',
  'thread-safety.md',
  'porting-0.8.md',
  'tests.md'
]

aravis_toml=configure_file(
  input: 'aravis.toml.in',
  output: 'aravis.toml',
  configuration: toml_conf,
  install: true,
  install_dir: docs_dir / 'aravis-@0@'.format(aravis_api_version)
)

custom_target(
  'aravis-doc',
  input: [ aravis_toml, aravis_gir[0] ],
  output: 'aravis-@0@'.format(aravis_api_version),
  command: [
    gidocgen,
    'generate',
    gidocgen_common_args,
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../../../src'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@'
    ],
    depend_files: [ expand_content_md_files ],
    build_by_default: true,
    install: true,
    install_dir: docs_dir
  )
