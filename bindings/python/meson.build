if get_option('python_bindings').enabled()
  py_mod = import('python')
  py = py_mod.find_installation('python3', modules: ['pytest'], required: true)

  test_env = environment()
  test_env.set('APG_LIBRARY_PATH', libapg.full_path())
  test_env.set('PYTHONPATH', meson.current_source_dir())

  sanitize = get_option('b_sanitize')
  cc = meson.get_compiler('c')
  if sanitize != 'none' and cc.get_id() == 'clang'
    fs = import('fs')
    clang = cc.cmd_array()[0]
    rt_dir = run_command(clang, '-print-runtime-dir', check: true).stdout().strip()
    runtimes = []
    if sanitize.contains('address')
      runtimes += 'asan'
      test_env.set('LSAN_OPTIONS',
        'suppressions=' + meson.current_source_dir() / 'lsan-suppressions.txt')
    endif
    if sanitize.contains('thread')
      runtimes += 'tsan'
    endif
    preload = []
    foreach runtime : runtimes
      rt_file = 'libclang_rt.' + runtime + '-' + host_machine.cpu_family() + '.so'
      rt_path = run_command(clang, '-print-file-name=' + rt_file,
                            check: true).stdout().strip()
      if not fs.exists(rt_path)
        rt_path = rt_dir / rt_file
      endif
      preload += rt_path
    endforeach
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
