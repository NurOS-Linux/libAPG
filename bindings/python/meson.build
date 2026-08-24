if get_option('python_bindings').enabled()
  py_mod = import('python')
  py = py_mod.find_installation('python3', modules: ['pytest'], required: true)

  test_env = environment()
  test_env.set('APG_LIBRARY_PATH', libapg.full_path())
  test_env.set('PYTHONPATH', meson.current_source_dir())

  sanitize = get_option('b_sanitize')
  cc = meson.get_compiler('c')
  if sanitize != 'none' and cc.get_id() == 'clang'
    rt_dir = run_command(cc.cmd_array()[0], '-print-runtime-dir',
                         check: true).stdout().strip()
    preload = []
    if sanitize.contains('address')
      preload += rt_dir / 'libclang_rt.asan-' + host_machine.cpu_family() + '.so'
    endif
    if sanitize.contains('thread')
      preload += rt_dir / 'libclang_rt.tsan-' + host_machine.cpu_family() + '.so'
    endif
    if preload.length() > 0
      test_env.set('LD_PRELOAD', ':'.join(preload))
    endif
  endif

  test('apgpy-bindings',
    py,
    args: ['-m', 'pytest', meson.current_source_dir() / 'tests', '-v'],
    env: test_env,
    timeout: 120,
  )
endif
