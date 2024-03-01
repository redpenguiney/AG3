sources = ['../external_headers/lua/onelua.cpp', 'graphics/framebuffer.cpp', 'gameobjects/transform_component.cpp', 'lua/lua_handler.cpp', 'graphics/engine.cpp', 'gameobjects/lifetime.cpp', 'physics/gjk.cpp', 'graphics/material.cpp', 'graphics/camera.cpp', 'physics/engine.cpp', 'gameobjects/rigidbody_component.cpp', 'physics/physics_mesh.cpp', 'graphics/shader_program.cpp', 'graphics/texture.cpp', 'graphics/mesh.cpp', 'graphics/window.cpp', 'graphics/renderable_mesh.cpp', 'graphics/meshpool.cpp', 'gameobjects/pointlight_component.cpp', 'graphics/buffered_buffer.cpp', 'physics/spatial_acceleration_structure.cpp', 'physics/raycast.cpp', 'gameobjects/component_registry.cpp', 'utility/utility.cpp']
glew_dep = declare_dependency(link_args: ['-lglew64'])
glfw_dep = declare_dependency(link_args: ['-l64glfw3'])
gl_dep = declare_dependency(link_args: ['-lopengl32'])
lua_dep = declare_dependency(link_args: ['-L -Llua54dll -ldl'])

incdir = include_directories('../external_headers/sol2/include', '../external_headers/lua')

srclib = static_library('srclib', sources, include_directories: incdir, dependencies: [lua_dep, glew_dep, glfw_dep, gl_dep], cpp_args: ['-O3', '-Wshadow'])