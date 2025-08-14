#pragma once
#include <map>
#include <memory>
#include <optional>
#include <glm/vec3.hpp>

#include <graphics/framebuffer.hpp>

class GameObject;
class Material;
class ShaderProgram;

// Uses the jump flood algorithm to let you put outlines/highlights around objects of your choice.
class HighlightHandler {
public:
	// TODO: config
	const static inline int GEOMETRY_DRAW_ORDER = 5000; // outlines will skip postprocessing
	//const static inline int UV_MASK_DRAW_ORDER = 5050;
	const static inline int JUMP0_DRAW_ORDER = 5051;
	const static inline int PRESENTATION_DRAW_ORDER = 5100;

	// Pointer must be valid for the whole lifetime of HighlightHandler, or replaced with nullptr when it becomes invalid.
	// If nullptr, it will be drawn to the default framebuffer (aka the screen).
	void SetTargetFramebuffer(Framebuffer* target);
	static HighlightHandler& Get();
	
	// FYI, will give the object an extra child transform named "__HIGHLIGHT__". 
	void AddHighlight(std::shared_ptr<GameObject> object, float outlineSize, glm::vec3 outlineColor);

	void RemoveHightlight(std::shared_ptr<GameObject> object);

private:

	HighlightHandler();
	~HighlightHandler();

	Framebuffer* presentationTarget = nullptr;

	// key is gameobject with a highlight, value is gameobject storing the highlight rendercomponent.
	// map instead of unordered_map bc can't hash weak_ptr :(
	std::map<std::weak_ptr<GameObject>, std::shared_ptr<GameObject>, std::owner_less<std::weak_ptr<GameObject>>> highlights;

	// jump flood algorithm needs a framebuffer to work on.
	// attachment0 is rgba for final output, attachment1 is grayscale geometry mask (which is probably useless and can be removed? TODO)
	std::optional<Framebuffer> highlightFramebuffer;

	// only has attachment0 (rgba for final output). 
	// We need a second one to ping-pong bc jump flood algorithm wants to read and write to the same framebuffer otherwise which is a big graphics no-no.
	std::optional<Framebuffer> highlightFramebuffer2;

	// materials used to render geometry of highlighted objects to attachment1; multiple because we need to accommodate varied vertex shaders
	std::vector<std::shared_ptr<Material>> geometryMaterials;

	// postprocessing-quad material 
	//std::shared_ptr<Material> uvMaskingMaterial;

	// postprocessing-quad materials; multiple of them because jumpflood requires roughly log2(outline_width) passes
	// first is size1, 
	std::vector<std::shared_ptr<Material>> jumpFloodMaterials;

	// used to put finished outline on screen
	std::shared_ptr<Material> presentationMaterial;

	static void GeometryPassInputProviderFunc(Material*, std::shared_ptr<ShaderProgram>);

	template<float stepSize, bool useSecond, bool clear>
	static void JumpFloodPassInputProviderFunc(Material*, std::shared_ptr<ShaderProgram>);

	template <float stepSize, bool writeSecond, bool clear = false>
	std::shared_ptr<Material> MakeJumpFloodPass(int passIndex);

	static void PresentationPassInputProvider(Material*, std::shared_ptr<ShaderProgram>);
};
