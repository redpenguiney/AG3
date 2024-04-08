#pragma once
#include "../gameobjects/component_registry.hpp"
#include <sol/sol.hpp>
#include "../../external_headers/GLM/gtx/string_cast.hpp"

struct LuaGameobjectCreateParams: GameobjectCreateParams {
    LuaGameobjectCreateParams(sol::lua_table args);
};

std::shared_ptr<GameObject> LuaGameobjectConstructor(sol::object args);

template<typename A, typename B>
A multiply(A a, B b) {
    return a * b;
}

template<typename A, typename B>
A division(A a, B b) {
    return a / b;
}

template<typename A, typename B>
A add(A a, B b) {
    return a + b;
}

template<typename A, typename B>
A subtract(A a, B b) {
    return a - b;
}

template<typename A>
std::string toString(A a) {
    return glm::to_string(a);
}

template<typename vecT, typename scalarT> 
void SetupVecUsertype(sol::state* state, const char* typeName) {
    auto vecUsertype = state->new_usertype<vecT>(typeName, sol::constructors<vecT(), vecT(double), vecT(float), vecT(double, double, double), vecT(float, float, float), vecT(glm::dvec3), vecT(glm::vec3)>());
    vecUsertype["x"] = &vecT::x;
    vecUsertype["y"] = &vecT::y;
    vecUsertype["z"] = &vecT::z;
    vecUsertype["__mul"] = sol::overload(multiply<vecT, scalarT>, multiply<vecT, vecT>);
    vecUsertype["__div"] = sol::overload(division<vecT, scalarT>, division<vecT, vecT>);
    vecUsertype["__add"] = sol::overload(add<vecT, scalarT>, add<vecT, vecT>);
    vecUsertype["__sub"] = sol::overload(subtract<vecT, scalarT>, subtract<vecT, vecT>);
    vecUsertype["__tostring"] = toString<vecT>;
}