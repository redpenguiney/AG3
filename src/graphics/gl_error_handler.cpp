#include "../../external_headers/GLEW/glew.h"
#include <cstdio>
#include <cstdlib>

// OpenGL automatically calls this for warnings/errors if GL_DEBUG_OUTPUT is enabled
// See window.cpp for GL_DEBUG_OUTPUT is setup
void GLAPIENTRY MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* _userParam )
{
  if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) {
      return; // Shaders do their own checking for compile errors, and checking here won't give us the nature of the compile error, just the fact that one occurred.
  }
  if (severity == GL_DEBUG_SEVERITY_MEDIUM || GL_DEBUG_SEVERITY_HIGH) {
      fprintf( stderr, "\nFATAL OPENGL ERROR: %s type = 0x%x, severity = 0x%x, message = %s",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
      std::printf("\nAborting.");
      abort();
  }
  else if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
      fprintf( stderr, "\n Minor OpenGL warning: %s type = 0x%x, severity = 0x%x, message = %s",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );

  }
}