sources = ['graphics/engine.cpp', 'graphics/camera.cpp', 'graphics/shader_program.cpp', 'graphics/texture.cpp', 'graphics/mesh.cpp', 'graphics/window.cpp', 'graphics/renderable_mesh.cpp', 'graphics/meshpool.cpp', 'graphics/buffered_buffer.cpp', 'physics/spatial_acceleration_structure.cpp', 'physics/raycast.cpp', 'gameobjects/gameobject.cpp', 'utility/utility.cpp']
glew_dep = declare_dependency(link_args: ['-lglew64'])
glfw_dep = declare_dependency(link_args: ['-l64glfw3'])
gl_dep = declare_dependency(link_args: ['-lopengl32'])

srclib = static_library('srclib', sources, dependencies: [glew_dep, glfw_dep, gl_dep])