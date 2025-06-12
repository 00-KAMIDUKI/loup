project(
  'loup',
  ['cpp', 'c'],
  meson_version: '>=1.8.0',
  default_options: [
    'buildtype=release',
    'cpp_std=c++26',
    'warning_level=3',
  ],
)

add_project_arguments(
  '-Wno-keyword-macro',
  language: 'cpp',
)

wayland = import('wayland')
wayland_targets = wayland.scan_xml(
  'wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml',
  wayland.find_protocol('xdg-shell'),
)

exe = executable(
  'loup',
  'main.cc',
  wayland_targets,
  dependencies: dependency('wayland-client'),
  install: true,
)

test('basic', exe)
