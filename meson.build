project('AG3', 'cpp', default_options : ['cpp_std=c++20', 'optimization=3', 'cpp_args=-Wshadow'])
glew_dep = declare_dependency(link_args: ['-lglew64'])
glfw_dep = declare_dependency(link_args: ['-l64glfw3'])
gl_dep = declare_dependency(link_args: ['-lopengl32'])
lua_dep = declare_dependency(link_args: ['-Lbin -llua54dll -ldl'])
#freetype_dep = declare_dependency(link_args: ['-Lbin -freetypedll'])

freetype_proj = subproject('freetype')
freetype_dep = freetype_proj.get_variable('freetype_dep')

subdir('src')

e = executable('AG3', 'src/main.cpp', link_with: srclib, dependencies: [lua_dep, glew_dep, glfw_dep, gl_dep, freetype_dep])
test('er', e)