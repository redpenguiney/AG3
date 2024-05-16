#pragma once
#include "gameobjects/component_registry.hpp"
#include "gameobjects/transform_component.hpp"

// Sadly, due to SOL limitations we can't have lua automatically/simply use ComponentHandle<whatever>, and have to have this extra wrapper. At least this way we can do prettier error checking if we want.

template<typename T>
class LuaComponentHandleWrapper {
    ComponentHandle<T>& handle;
    operator ComponentHandle<T>();
};

// class LuaTransformComponent: ComponentHandle<TransformComponent> {
//     public:
//     glm::dvec3 LuaGetPos();
//     void LuaSetPos(const glm::vec3);
//     glm::quat LuaGetRot();
//     void LuaSetRot(const glm::quat);
//     glm::vec3 LuaGetScl();
//     void LuaSetScl(const glm::vec3);
//     LuaTransformComponent& LuaGetParent(); 
//     void LuaSetParent(const LuaTransformComponent&);

//     LuaTransformComponent(const ComponentHandle<TransformComponent>&);
//     // operator ComponentHandle<TransformComponent>();
// };

