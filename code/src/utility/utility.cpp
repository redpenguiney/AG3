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

void DebugPlacePointOnPosition(glm::dvec3 position, glm::vec4 color) {
    // TODO: cache filename so stuff like this isn't so bad
    static auto m = Mesh::MultiFromFile("../models/rainbowcube.obj", MeshCreateParams{.expectedCount = 16384}).back().mesh;
    GameobjectCreateParams params({ComponentBitIndex::Transform, ComponentBitIndex::Render});
    params.meshId = m->meshId;
    auto g = GameObject::New(params);
    g->RawGet<TransformComponent>()->SetPos(position);
    g->RawGet<TransformComponent>()->SetScl({0.1, 0.1, 0.1});
    g->RawGet<RenderComponent>()->SetColor(color);
    g->RawGet<RenderComponent>()->SetTextureZ(-1);
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
