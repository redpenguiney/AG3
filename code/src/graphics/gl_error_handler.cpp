#pragma once
#include "GL/glew.h"
#include <cstdio>
#include "debug/log.hpp"

// OpenGL automatically calls this for warnings/errors if GL_DEBUG_OUTPUT is enabled
// See window.cpp for GL_DEBUG_OUTPUT is setup
inline void GLAPIENTRY MessageCallback( GLenum source,
                 GLenum type,
                 GLuint,
                 GLenum severity,
                 GLsizei,
                 const GLchar* message,
                 const void*)
{
  if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) {
      return; // Shaders do their own checking for compile errors, and checking here won't give us the nature of the compile error, just the fact that one occurred.
  }
  if (type == GL_DEBUG_TYPE_ERROR && (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_HIGH)) {
      DebugLogError("Fatal OpenGL error: type ", type, ", severity ", severity, ", message \"", message, "\".\nAborting.");
      abort();
  }
  else if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
      DebugLogInfo("Minor OpenGL error: type ", type, ", severity ", severity, ", message \"", message, "\".");
  }
}