#include "debug/log.hpp"
#include <conglomerates/basic_renderer.hpp>
#include <graphics/gengine.hpp>

void GameInit() {
	DebugLogInfo("SANDBOXING? MORE LIKE SANDBAGGING!");

	BasicRenderer::Setup();
	GraphicsEngine::Get().SetDebugFreecamEnabled(true);
}

void GameClose() {}