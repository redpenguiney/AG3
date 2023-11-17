#include "material.hpp"
#include <cassert>

void Material::Destroy(const unsigned int id) {
    assert(MATERIALS.count(id));
    MATERIALS.erase(id);
}

void Material::Use() {
    color->Use();
    if (specular) specular->Use();
    if (normal) normal->Use();
}