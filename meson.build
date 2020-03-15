project('libkikaitachi', 'c')

inc = include_directories('include')
libkikaitachi = shared_library('kikaitachi',
  'src/kikaitachi.c',
  include_directories : inc,
  install : true)

libsimple_dep = declare_dependency(
  include_directories : inc,
  link_with : libkikaitachi)

