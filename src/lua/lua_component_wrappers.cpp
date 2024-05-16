#include "lua_component_wrappers.hpp"

// glm::dvec3 LuaTransformComponent::LuaGetPos() {
//     return (**this).Position();
// }
// void LuaTransformComponent::LuaSetPos(const glm::vec3 pos) {
//     (**this).SetPos(pos);
// }
// glm::quat LuaTransformComponent::LuaGetRot() {
//     return (**this).Rotation();
// }
// void LuaTransformComponent::LuaSetRot(const glm::quat rot) {
//     (**this).SetRot(rot);
// }
// glm::vec3 LuaTransformComponent::LuaGetScl() {
//     return (**this).Scale();
// }
// void LuaTransformComponent::LuaSetScl(const glm::vec3 scl) {
//     (**this).SetScl(scl);
// }
// LuaTransformComponent& LuaTransformComponent::LuaGetParent() {
//     return ComponentHandle((**this).GetParent());
// }
// void LuaTransformComponent::LuaSetParent(const LuaTransformComponent& newParent) {
//     (**this).SetParent(*newParent);
// }

// LuaTransformComponent::LuaTransformComponent(const ComponentHandle<TransformComponent>&):  {

// }