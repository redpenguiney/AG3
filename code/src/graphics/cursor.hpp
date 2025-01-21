#pragma once
#include <memory>
#include "texture.hpp"
#include <GLFW/glfw3.h>

// Class for using custom or system-provided cursors.
// Use the Window class to change which cursor is rendered at any given time.
// The Window class stores
class Cursor {
public:
	// Generates a custom cursor using the image provided by the TextureSource object. (TODO)
	// Throws std::runtime_error if glfwCreateCursor fails (which could happen for various runtime/device-environment-specific reasons).
	Cursor(const TextureSource&);

	
	Cursor(const Cursor&) = delete;
	Cursor(Cursor&&);
	Cursor& operator=(Cursor&& other);

	~Cursor() noexcept;

private:

	// Creates an invalid cursor that CANNOT be used. Just for Window's convienecnce.
	Cursor();

	// for use by Window to generate system-provided cursors.
	Cursor(GLenum systemCursorType);

	friend class Window;

	GLFWcursor* cursor;
};
