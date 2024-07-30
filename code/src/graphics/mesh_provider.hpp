#pragma once
#include <utility>
#include <memory>
#include <vector>

// When you create a Mesh, it needs to get its vertices and indices from somewhere.
// When loading from files via assimp, you use Mesh::MultiFromFile().
	// This is because assimp decides basically everything about the meshes (yes, plural which is why assimp file loading doesn't use MeshProvider).
// For other stuff, you use Mesh::New() and provide a object that inherits from MeshProvider.
class MeshProvider {
public:
	// returns the vertices and indices.
	virtual std::pair < std::unique_ptr < std::vector < float >> , std::unique_ptr<std::vector< unsigned int>> > GetMesh() = 0;

	virtual ~MeshProvider() = default;
};

// Mesh provider that takes user-specified vertices and indices.
class RawMeshProvider {
public:
	RawMeshProvider(std::vector<float>&& vertices, std::vector<unsigned int>&& indices);

	std::pair < std::unique_ptr < std::vector < float >>, std::unique_ptr<std::vector< unsigned int>> > GetMesh();

	std::vector<float> vertices;
	std::vector<unsigned int> indices;
};