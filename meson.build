project('libkikaitachi', 'c')

inc = include_directories('include')

glib_dep = dependency('glib-2.0')
v4l_dep = dependency('libv4l2')

libkikaitachi = shared_library('kikaitachi',
  'src/kikaitachi.c',
  dependencies: [v4l_dep],
  include_directories : inc,
  install : true)

libkikaitachi_dep = declare_dependency(
  include_directories : inc,
  link_with : libkikaitachi)

test(
  'tests',
  executable('tests', 'test/tests.c', include_directories : inc, dependencies: [glib_dep, v4l_dep, libkikaitachi_dep]),
  env: [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  ],
)

