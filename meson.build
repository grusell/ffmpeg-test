project('ffmpeg-test', 'cpp', default_options : ['cpp_std=c++11','warning_level=3'])

catch2_proj = subproject('catch2')

avlibs = [
  'libavformat',
  'libavcodec',
  'libavutil',
  'libavfilter',
  'libswscale'
]

FFMPEG_DIR = '/home/gugr01/gitrepos/private/FFmpeg/'

cc = meson.get_compiler('cpp')
deps = []

foreach l : avlibs
  new_dep = cc.find_library(l, dirs: [FFMPEG_DIR + l])
  deps += [new_dep]
endforeach


test_deps = [
  catch2_proj.get_variable('catch2_dep')
]

incdir = include_directories('include')

seekTest= executable('seekTest', 'test/seek_test.cc',  dependencies: deps + test_deps, include_directories: incdir)
test('seek', seekTest)
