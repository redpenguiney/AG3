#include "cursor.hpp"
#include <debug/assert.hpp>
#include "window.hpp"

Cursor::Cursor(GLenum systemCursorType) {
	cursor = glfwCreateStandardCursor(systemCursorType); // standard cursor creation shold NOT fail.
	Assert(cursor != nullptr);
}

Cursor::Cursor(Cursor&& old)
{
	cursor = old.cursor;
	old.cursor = nullptr;
}

Cursor& Cursor::operator=(Cursor&& other)
{

	cursor = other.cursor;
	other.cursor = nullptr;
	return *this;
}

Cursor::Cursor() {
	cursor = nullptr;
}

Cursor::~Cursor() noexcept {
	if (!Window::GLFW_INIT) return; // can't/needn't destroy glfw stuff if glfw was already terminated
	if (cursor != nullptr)
		glfwDestroyCursor(cursor);
}