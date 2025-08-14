#pragma once
#include <memory>
#include <optional>
#include <graphics/framebuffer.hpp>
#include <glm/ext.hpp>

class GameObject;
class ShaderProgram;
class Material;

// A simple renderer with support for postprocessing and order-independent transparency (OIT). You'll need to create your own equivalent for this if you want rendering to work.
// Plays nice with the functions of skybox_factory.cpp.
// TODO: WIREFRAME FIX
class BasicRenderer {
public:
	static const int OPAQUE_DRAW_ORDER = 0;
	static const int PREDRAW_TRANSPARENT_DRAW_ORDER = 1000;
	static const int TRANSPARENT_DRAW_ORDER = 1500;
	static const int TRANSPARENT_COMPOSITION_DRAW_ORDER = 2000;
	static const int POSTPROC_DRAW_ORDER = 3000; // use a number bigger than this to make something skip postprocessing
	static const int PREDRAW_DRAW_ORDER = -1000000;

	// Call once before rendering begins.
	static BasicRenderer& Setup(std::shared_ptr<ShaderProgram> postProcShader = nullptr);

	static std::shared_ptr<Material>& GetDefaultTransparentMaterial();

	// true if Setup() has been called.
	static inline bool used = false;

	// everything is drawn onto this framebuffer
	// todo: make it not do post processing when this isn't here
	// has color and depth, plus accumulation + revealage for use in OIT
	std::optional<Framebuffer> mainFramebuffer;
	
	// tells the renderer this shader needs lighting data/etc.
	void AddShader(std::shared_ptr<ShaderProgram> shader);

	static void PrepPostprocessing(Material* material, std::shared_ptr<ShaderProgram> _);
	static void PrepNormalRendering(Material* material, std::shared_ptr<ShaderProgram> _);
	static void PreRendering(Material*, std::shared_ptr<ShaderProgram>);
	static void PreTransparentRendering(Material*, std::shared_ptr<ShaderProgram>);
	// Used for OIT
	//std::shared_ptr<GameObject> compositionScreenQuad;

	// Final scene is redrawn onto this quad
	//std::shared_ptr<GameObject> postProcScreenQuad;

	struct EnvironmentalLighting {
		glm::vec3 dir = glm::normalize(-glm::vec3(0.9, -1, 0.8));
		glm::vec3 color = glm::vec3(237.f / 255, 213.f / 255, 158.f / 255);
		float ambientStrength = 0.4f;
		float diffuseStrength = 0.9f;
		float specularStrength = 0.0f;
	};

	void SetEnvironmentalLighting(const EnvironmentalLighting&);

private:
	BasicRenderer(std::shared_ptr<ShaderProgram> postProcShader);

	std::vector<std::weak_ptr<ShaderProgram>> shaders;

	EnvironmentalLighting currentLighting = EnvironmentalLighting();
};