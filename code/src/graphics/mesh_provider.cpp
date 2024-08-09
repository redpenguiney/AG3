#include "mesh_provider.hpp"
#include "mesh.cpp"

MeshCreateParams MeshCreateParams::Default() {
    return MeshCreateParams();
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    return MeshCreateParams({ .meshVertexFormat = MeshVertexFormat::DefaultGui() });
}

RawMeshProvider::RawMeshProvider(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, const MeshCreateParams& params):
    vertices(vertices),
    indices(indices),
    MeshProvider(params)
{
}

std::pair<std::vector<float>, std::vector<unsigned int>> RawMeshProvider::GetMesh() const
{
    return std::make_pair(vertices, indices);
}

TextMeshProvider::TextMeshProvider(const MeshCreateParams& params) : MeshProvider(params)
{
}



MeshProvider::MeshProvider(const MeshCreateParams& p) : meshParams(p)
{
}
