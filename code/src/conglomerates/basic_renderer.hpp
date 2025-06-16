#pragma once
#include <memory>
#include <optional>
#include <graphics/framebuffer.hpp>

class GameObject;
class ShaderProgram;
class Material;

// A simple renderer with support for postprocessing and order-independent transparency (OIT). You'll need to create your own equivalent for this if you want rendering to work.
// Plays nice with the functions of skybox_factory.cpp.
// TODO: WIREFRAME FIX
class BasicRenderer {
public:
	static const int OPAQUE_DRAW_ORDER = 0;
	static const int TRANSPARENT_DRAW_ORDER = 1000;
	static const int TRANSPARENT_COMPOSITION_DRAW_ORDER = 1500;
	static const int POSTPROC_DRAW_ORDER = 2000; // use a number bigger than this to make something skip postprocessing

	// Call once before rendering begins.
	static BasicRenderer& Setup(std::shared_ptr<ShaderProgram> postProcShader = nullptr);

	// true if Setup() has been called.
	static inline bool used = false;

	// everything is drawn onto this framebuffer
	// todo: make it not do post processing when this isn't here
	// has color and depth, plus accumulation + revealage for use in OIT
	std::optional<Framebuffer> mainFramebuffer;

	static void PrepPostprocessing(Material* material, std::shared_ptr<ShaderProgram> _);
	static void PrepNormalRendering(Material* material, std::shared_ptr<ShaderProgram> _);
	// Used for OIT
	//std::shared_ptr<GameObject> compositionScreenQuad;

	// Final scene is redrawn onto this quad
	//std::shared_ptr<GameObject> postProcScreenQuad;

private:
	BasicRenderer(std::shared_ptr<ShaderProgram> postProcShader);

	
};