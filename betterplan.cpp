#include <cstdlib>
#include <vector>
struct RenderComponent {

};

struct TransformComponent {

};

struct ColliderComponent {

};

struct RigidbodyComponent {

};

std::vector<RenderComponent> RENDER_COMPONENTS;
std::vector<TransformComponent> TRANSFORM_COMPONENTS;
std::vector<ColliderComponent> COLLIDER_COMPONENTS;
std::vector<RigidbodyComponent> RIGIDBODY_COMPONENTS;

void UpdateRenderComponents() {
    for (auto & comp: RENDER_COMPONENTS) {
        
    }
}

int main() {


    return EXIT_SUCCESS;
}