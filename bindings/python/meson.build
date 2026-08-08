if get_option('python_bindings').enabled()
  py_mod = import('python')
  py = py_mod.find_installation('python3', modules: ['pytest'], required: true)

  test('apgpy-bindings',
    py,
    args: ['-m', 'pytest', meson.current_source_dir() / 'tests', '-v'],
    env: {
      'APG_LIBRARY_PATH': libapg.full_path(),
      'PYTHONPATH': meson.current_source_dir(),
    },
    timeout: 120,
  )
endif
