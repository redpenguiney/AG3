#include <cmath>
#include <cstdio>
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
    static auto m = Mesh::FromFile("../models/rainbowcube.obj", MeshVertexFormat::Default(), -1.0, 1.0, 16384);
    GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
    params.meshId = m->meshId;
    auto g = ComponentRegistry::NewGameObject(params);
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