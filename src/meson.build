smen_sources = [
  'smen.cpp',
  'engine.cpp',
  'window.cpp',
  'renderer.cpp',
  'session.cpp',
  'texture.cpp'
]

subdir('lua')
subdir('ecs')
subdir('variant')
subdir('ser')
subdir('gui')
subdir('smen_library')

smen_exe = executable(
  'smen',
  smen_sources,
  dependencies: [ sdl2_dep, sdl2ttf_dep, sdl2image_dep, sdl2mixer_dep, luajit_dep, gl_dep, imgui_dep ],
  include_directories: includes,
  install: true
)

