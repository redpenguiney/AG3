#include "mesh_simplification/simplify.cpp"
#include <vector>

// Declared in physics_mesh.cpp
void SimplifyMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, float threshold) {
    // Simplify::Vertex;
    // Simplify::vertices = vertices;
    Simplify::simplify_mesh(float(indices.size())/3.0f/threshold, 7);
}