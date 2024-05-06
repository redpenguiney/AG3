project('AG3', 'cpp', default_options : ['cpp_std=c++20', 'optimization=3', 'cpp_args=-Wshadow -Wall'])
#glew_dep = declare_dependency(link_args: ['-lglew64'])
#glfw_dep = declare_dependency(link_args: ['-l64glfw3'])
#gl_dep = declare_dependency(link_args: ['-lopengl32'])
#lua_dep = declare_dependency(link_args: ['-Lbin -llua54dll -ldl'])
#freetype_dep = declare_dependency(link_args: ['-Lbin -freetypedll'])

freetype_dep = subproject('freetype').get_variable('freetype_dep')
glew_dep = subproject('glew').get_variable('glew_dep')
glfw_dep = subproject('glfw').get_variable('glfw_dep')
#gl_dep = dependency('opengl', method : 'system')
lua_dep = subproject('lua').get_variable('lua_dep')
openalsoft_dep = subproject('openal-soft').get_variable('openal_dep')

subdir('src')

e = executable('AG3', 'src/main.cpp', link_with: srclib, dependencies: [lua_dep, glew_dep, glfw_dep, freetype_dep, openalsoft_dep])
test_ext = shared_library('test_module', link_with: srclib)
test('er', e)