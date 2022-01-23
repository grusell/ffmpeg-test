project('ffmpeg-test', 'cpp', default_options : ['cpp_std=c++11','warning_level=3'])

catch2_proj = subproject('catch2')

deps = [
  dependency('libavformat'),
  dependency('libavcodec'),
  dependency('libavutil'),
  dependency('libavfilter'),
  dependency('libswscale')
]

test_deps = [
  catch2_proj.get_variable('catch2_dep')
]

incdir = include_directories('include')


# executable('vivictpp', 'src/main.cc', link_with: vivictpplib, dependencies: deps, include_directories: incdir, install: true)

seekTest= executable('seekTest', 'test/seek_test.cc',  dependencies: deps + test_deps, include_directories: incdir)
test('seek', seekTest)
# test('FormatHandler.seek', seekTest)
#playbackTest= executable('playbackTest', 'test/PlaybackTest.cc', link_with: vivictpplib,  dependencies: deps + test_deps, include_directories: incdir)
#test('Playback', playbackTest)
