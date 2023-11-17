#include "material.hpp"
#include <cassert>
#include <memory>

void Material::Destroy(const unsigned int id) {
    assert(MATERIALS.count(id));
    MATERIALS.erase(id);
}

std::pair<float, std::shared_ptr<Material>> New(const std::vector<TextureCreateParams>& textureParams) {
    
}

void Material::Use() {
    color->Use();
    if (specular) specular->Use();
    if (normal) normal->Use();
}