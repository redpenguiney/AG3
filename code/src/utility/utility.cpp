#include <cmath>
#include <cstdio>
#include "graphics/mesh.hpp"
#include "utility.hpp"
#include "../gameobjects/component_registry.hpp"
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
    static auto m = std::get<0>(Mesh::MultiFromFile("../models/rainbowcube.obj", MeshCreateParams{.expectedCount = 16384}).back());
    GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
    params.meshId = m->meshId;
    auto g = ComponentRegistry::Get().NewGameObject(params);
    g->transformComponent->SetPos(position);
    g->transformComponent->SetScl({0.1, 0.1, 0.1});
    g->renderComponent->SetColor(color);
    g->renderComponent->SetTextureZ(-1);
}

double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = high_resolution_clock::now().time_since_epoch();
    return time.count()/1000.0;
}