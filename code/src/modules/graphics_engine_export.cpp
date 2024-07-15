#include "graphics_engine_export.hpp"

#include "graphics/gengine.hpp"

// idk why i need this
ModuleGraphicsEngineInterface::~ModuleGraphicsEngineInterface() {}

ModuleGraphicsEngineInterface* ModuleGraphicsEngineInterface::Get() {
    return &GraphicsEngine::Get();
}