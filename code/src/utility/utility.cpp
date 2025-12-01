#include <cmath>
#include <cstdio>
#include "graphics/mesh.hpp"
#include "utility.hpp"
#include "../gameobjects/gameobject.hpp"
#include <chrono>

glm::dvec3 LookVector(double pitch, double yaw) {
    return glm::dvec3(
        sin(yaw) * cos(pitch),
        -sin(pitch),
        -cos(yaw) * cos(pitch)
        
    );
}

std::shared_ptr<GameObject> DebugPlacePointOnPosition(glm::dvec3 position, glm::vec4 color) {
    // TODO: cache filename so stuff like this isn't so bad
    static auto m = Mesh::MultiFromFile("../models/rainbowcube.obj", MeshCreateParams{.expectedCount = 16384}).back().mesh;
    GameobjectCreateParams params({ComponentBitIndex::Transform, ComponentBitIndex::Render});
    params.meshId = m->meshId;
    auto g = GameObject::New(params);
    g->RawGet<TransformComponent>()->SetPos(position);
    g->RawGet<TransformComponent>()->SetScl({0.1, 0.1, 0.1});
    g->RawGet<RenderComponent>()->SetColor(color);
    g->RawGet<RenderComponent>()->SetTextureZ(-1);
    return g;
}

std::shared_ptr<Material> GetLinesMaterial() {
    MaterialCreateParams params{
        .shader = ShaderProgram::New("../shaders/debug_simple_vertex.glsl", "../shaders/debug_simple_fragment.glsl")
    };
    return Material::New(params).second;
}

std::shared_ptr<GameObject> DebugPlaceTriangle(std::array<glm::dvec3, 3> positions, glm::vec4 color)
{
    std::vector<GLfloat> vertices;
    for (auto& p : positions) {
        vertices.push_back(p[0]);
        vertices.push_back(p[1]);
        vertices.push_back(p[2]);
    }
    MeshVertexFormat format(MeshVertexFormat::FormatVertexAttributes{ 
        .position = VertexAttribute {.nFloats = 3, .instanced = false}, 
        .color = VertexAttribute {.nFloats = 4, .instanced = true}, 
        .modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
        .normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true} });
    format.primitiveType = GL_LINES;
    MeshCreateParams params;
    params.meshVertexFormat.emplace(format);
    params.normalizeSize = false;
    RawMeshProvider triMesh(vertices, { 0, 1, 1, 2, 2, 0 }, params);
    auto m = Mesh::New(triMesh);
    GameobjectCreateParams goparams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    goparams.meshId = m->meshId;
    static auto mat = GetLinesMaterial();
    goparams.materialId = mat->id;
    auto g = GameObject::New(goparams);
    g->RawGet<RenderComponent>()->SetColor(color);
    return g;
}

std::shared_ptr<GameObject> DebugPlaceLine(glm::dvec3 a, glm::dvec3 b, glm::vec4 color)
{
    std::vector<GLfloat> vertices;
    vertices.push_back(a[0]);
    vertices.push_back(a[1]);
    vertices.push_back(a[2]);
    vertices.push_back(b[0]);
    vertices.push_back(b[1]);
    vertices.push_back(b[2]);
    MeshVertexFormat format(MeshVertexFormat::FormatVertexAttributes{
        .position = VertexAttribute {.nFloats = 3, .instanced = false},
        .color = VertexAttribute {.nFloats = 4, .instanced = true},
        .modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
        .normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true} });
    format.primitiveType = GL_LINES;
    MeshCreateParams params;
    params.meshVertexFormat.emplace(format);
    params.normalizeSize = false;
    RawMeshProvider triMesh(vertices, { 0, 1 }, params);
    auto m = Mesh::New(triMesh);
    GameobjectCreateParams goparams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    goparams.meshId = m->meshId;
    static auto mat = GetLinesMaterial();
    goparams.materialId = mat->id;
    auto g = GameObject::New(goparams);
    g->RawGet<RenderComponent>()->SetColor(color);
    return g;
}

double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = high_resolution_clock::now().time_since_epoch();
    return time.count()/1000.0;
}

IdProvider::IdProvider()
{
    largestId = 0;
}

void IdProvider::ReturnId(unsigned int id)
{
    freeIds.push_back(id);
}

unsigned int IdProvider::GetId()
{
    if (freeIds.size()) {
        return freeIds.back();
        freeIds.pop_back();
    }
    else {
        return largestId++;
    }
}
