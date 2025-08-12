if host_machine.system()=='linux'
  kernel_version = run_command('uname', '-r').stdout().strip()
  kernel_build_dir = join_paths('/', 'lib', 'modules', kernel_version, 'build')

  # test running make in kernel directory, using "make kernelversion"
  make_returncode = run_command('make', '-sC', kernel_build_dir, 'kernelversion').returncode()
  if make_returncode != 0
    error('Cannot compile kernel modules as requested - are kernel headers installed?')
  endif

  subdir('linux-gv')
endif
