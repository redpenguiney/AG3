#include "modules/component_registry_export.hpp"
#include "gameobjects/component_registry.hpp"

ModuleComponentRegistryInterface::~ModuleComponentRegistryInterface() {}

ModuleComponentRegistryInterface* ModuleComponentRegistryInterface::Get() {
    // return &ComponentRegistry::Get();
    return nullptr;
}