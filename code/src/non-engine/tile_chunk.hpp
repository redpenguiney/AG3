#pragma once
#include <GLM/vec2.hpp>
#include <memory>

class TextureAtlas;
class GameObject;
class Mesh;
class Material;

class RenderChunk {
public:
	RenderChunk(glm::ivec2 centerPos, int stride, int radius, std::shared_ptr<Material>& material, std::shared_ptr<TextureAtlas>& atlas);

	RenderChunk(const RenderChunk&) = delete;

	~RenderChunk();
private:
	std::shared_ptr<Material> material;
	std::shared_ptr<TextureAtlas> atlas;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<GameObject> object;

	void MakeMesh(glm::ivec2 centerPos, int stride, int radius);
};