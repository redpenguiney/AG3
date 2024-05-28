project('AG3', 'cpp', default_options : ['cpp_std=c++20', 'optimization=3', 'cpp_args=-Wshadow -Wall -static'])
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

#subdir('src')

engine_sources = ['src/physics/mesh_simplification.cpp', 'external_headers/lua/onelua.cpp', 'src/graphics/stb_image.cpp', 'src/graphics/tinyobjloader.cpp', 'src/gameobjects/render_component.cpp', 'src/gameobjects/collider_component.cpp', 'src/lua/lua_component_wrappers.cpp', 'src/audio/sound.cpp', 'src/audio/engine.cpp', 'src/modules/module.cpp', 'src/lua/lua_constructor_wrappers.cpp', 'src/gameobjects/audio_player_component.cpp', 'src/conglomerates/gui.cpp', 'src/graphics/framebuffer.cpp', 'src/gameobjects/transform_component.cpp', 'src/lua/lua_handler.cpp', 'src/graphics/engine.cpp', 'src/gameobjects/lifetime.cpp', 'src/physics/gjk.cpp', 'src/graphics/material.cpp', 'src/graphics/camera.cpp', 'src/physics/engine.cpp', 'src/gameobjects/rigidbody_component.cpp', 'src/physics/physics_mesh.cpp', 'src/graphics/shader_program.cpp', 'src/graphics/texture.cpp', 'src/graphics/mesh.cpp', 'src/graphics/window.cpp', 'src/graphics/renderable_mesh.cpp', 'src/graphics/meshpool.cpp', 'src/gameobjects/pointlight_component.cpp', 'src/graphics/buffered_buffer.cpp', 'src/physics/spatial_acceleration_structure.cpp', 'src/physics/raycast.cpp', 'src/gameobjects/component_registry.cpp', 'src/utility/utility.cpp']

#todo pch would be nice, try to get this to work: cpp_pch : 'pch/external_pch.h',

incdir = include_directories('src', 'external_headers/sol2/include', 'external_headers/lua', 'external_headers', 'subprojects/freetype/include')
srclib = static_library('srclib', engine_sources, include_directories: incdir, dependencies: [lua_dep, glew_dep, glfw_dep, freetype_dep, openalsoft_dep], cpp_args: ['-O3', '-Wshadow', '-Wall', '-static', '-static-libgcc', '-static-libstdc++', '-DSOL_ALL_SAFETIES_ON=1'])
e = executable('AG3', 'src/main.cpp', link_with: srclib, include_directories: incdir, dependencies: [lua_dep, glew_dep, glfw_dep, freetype_dep, openalsoft_dep], cpp_args: ['-O3', '-Wshadow', '-Wall', '-static', '-static-libgcc', '-static-libstdc++', '-DSOL_ALL_SAFETIES_ON=1'])
mod = shared_library
#test('er', e)