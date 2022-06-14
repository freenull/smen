project(
  'smen',
  'cpp',
  default_options: [
    'cpp_std=c++20',
  ],
  version: '0.1',
  license: 'MIT',
  subproject_dir: 'subprojects'
)

includes = include_directories('include')

luajit_lib = meson.get_compiler('cpp').find_library('luajit-5.1')
luajit_dep = declare_dependency(dependencies: [ luajit_lib ])

sdl2_lib = meson.get_compiler('cpp').find_library('SDL2')
sdl2_dep = declare_dependency(dependencies: [ sdl2_lib ])

sdl2ttf_lib = meson.get_compiler('cpp').find_library('SDL2_ttf')
sdl2ttf_dep = declare_dependency(dependencies: [ sdl2ttf_lib ])

sdl2image_lib  = meson.get_compiler('cpp').find_library('SDL2_image')
sdl2image_dep  = declare_dependency(dependencies: [ sdl2image_lib ])

sdl2mixer_lib  = meson.get_compiler('cpp').find_library('SDL2_mixer')
sdl2mixer_dep  = declare_dependency(dependencies: [ sdl2mixer_lib ])

gl_lib = meson.get_compiler('cpp').find_library('GL')
gl_dep = declare_dependency(dependencies: [ gl_lib ])

imgui_proj = subproject('imgui')
imgui_dep = imgui_proj.get_variable('imgui_dep')

subdir('src')

