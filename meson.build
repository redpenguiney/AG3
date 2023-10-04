project('AG3', 'cpp', default_options : ['cpp_std=c++20'])
glew_dep = declare_dependency(link_args: ['-lglew64'])
gl_dep = dependency('opengl32')

subdir('src')

e = executable('AG3', 'src/main.cpp', link_with: srclib)
test('er', e)