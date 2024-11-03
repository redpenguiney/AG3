#pragma once
#include <GLM/vec2.hpp>
#include <memory>
#include <vector>

class TextureAtlas;
class GameObject;
class Mesh;
class Material;

class RenderChunk {
public:
	RenderChunk(glm::ivec2 centerPos, int stride, int radius, const std::shared_ptr<Material>& material, const std::shared_ptr<TextureAtlas>& atlas);

	RenderChunk(const RenderChunk&) = delete;

	~RenderChunk();

	bool dead = false;
private:
	const glm::ivec2 pos;

	std::shared_ptr<Material> material;
	std::shared_ptr<TextureAtlas> atlas;
	std::shared_ptr<Mesh> mesh;

	std::shared_ptr<GameObject> mainObject; // main terrain is just textured tiles and is shoved into one mesh / object
	std::vector<std::shared_ptr<GameObject>> objects; // other things have more complex meshes and have their own object

	void MakeMesh(glm::ivec2 centerPos, int stride, int radius);
};