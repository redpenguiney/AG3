#pragma once
#include "../gameobjects/component_registry.hpp"
#include <sol/sol.hpp>
#include "../../external_headers/GLM/gtx/string_cast.hpp"

struct LuaGameobjectCreateParams: GameobjectCreateParams {
    LuaGameobjectCreateParams(sol::lua_table args);
};

// We can't use normal ComponentHandle because sol would, if told it was like a pointer (which it is and it needs to know that), it would try to copy it.
template <typename T>
class LuaComponentHandle {
    public:
    ComponentHandle<T>* handle;

};

// lets sol recognize LuaComponentHandle as a pointer-like object, see https://sol2.readthedocs.io/en/latest/api/unique_usertype_traits.html

namespace sol {
        template <typename T>
        struct unique_usertype_traits<LuaComponentHandle<T>> {
                typedef T type;
                typedef LuaComponentHandle<T> actual_type;
                static const bool value = true;

                static bool is_null(const actual_type& ptr) {
                        return ptr.handle->GetPtr() == nullptr;
                }

                static type* get (const actual_type& ptr) {
                        return ptr.handle->GetPtr();
                }
        };
}

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
    vecUsertype["x"] = sol::readonly(&vecT::x);
    vecUsertype["y"] = sol::readonly(&vecT::y);
    vecUsertype["z"] = sol::readonly(&vecT::z);
    vecUsertype["__mul"] = sol::overload(multiply<vecT, scalarT>, multiply<vecT, vecT>);
    vecUsertype["__div"] = sol::overload(division<vecT, scalarT>, division<vecT, vecT>);
    vecUsertype["__add"] = sol::overload(add<vecT, scalarT>, add<vecT, vecT>);
    vecUsertype["__sub"] = sol::overload(subtract<vecT, scalarT>, subtract<vecT, vecT>);
    vecUsertype["__tostring"] = toString<vecT>;
}