#pragma once
#include "graphics/gengine.hpp"
#include <graphics/meshpool.hpp>

// Gameobjects that want to be rendered should have a pointer to one of these.
// However, they are stored here in a vector because that's better for the cache. (google ECS).
// NEVER DELETE THIS POINTER, JUST CALL Destroy(). DO NOT STORE OUTSIDE A GAMEOBJECT. THESE USE AN OBJECT POOL.
class RenderComponent: public BaseComponent {
public:

    const unsigned int materialId;
    const unsigned int meshId;

    RenderComponent(const RenderComponent&) = delete;
    RenderComponent(unsigned int meshId, unsigned int materialId);
    ~RenderComponent();

    // called to initialize when given to a gameobject
    //void Init(unsigned int meshId, unsigned int materialId, unsigned int shaderId = GraphicsEngine::Get().defaultShaderProgram->shaderProgramId);

    // call before returning to pool
    //void Destroy();

    // Sets the instanced vertex attribute (which depends on the mesh, but could be color, textureZ, etc.)
    // Note: setting color, textureZ, etc. is complicated because A. needs to be fast for all meshes that have different instanced vertex attributes B. multibuffering means that to set color, one must set color 3 times over 3 frames C. we can't set those things until the render component gets its meshpool, and for perf reasons we cache mesh creation so that doesn't happen until RenderScene() is called.
    //void SetInstancedVertexAttribute(const unsigned int attributeName, const glm::vec4& value);

    //// Sets the instanced vertex attribute (which depends on the mesh, but could be color, textureZ, etc.)
    //// Note: setting color, textureZ, etc. is complicated because A. needs to be fast for all meshes that have different instanced vertex attributes B. multibuffering means that to set color, one must set color 3 times over 3 frames C. we can't set those things until the render component gets its meshpool, and for perf reasons we cache mesh creation so that doesn't happen until RenderScene() is called.
    //void SetInstancedVertexAttribute(const unsigned int attributeName, const glm::vec3& value);

    //// Sets the instanced vertex attribute (which depends on the mesh, but could be color, textureZ, etc.)
    //// Note: setting color, textureZ, etc. is complicated because A. needs to be fast for all meshes that have different instanced vertex attributes B. multibuffering means that to set color, one must set color 3 times over 3 frames C. we can't set those things until the render component gets its meshpool, and for perf reasons we cache mesh creation so that doesn't happen until RenderScene() is called.
    //void SetInstancedVertexAttribute(const unsigned int attributeName, const glm::vec2& value);

    //// Sets the instanced vertex attribute (which depends on the mesh, but could be color, textureZ, etc.)
    //// Note: setting color, textureZ, etc. is complicated because A. needs to be fast for all meshes that have different instanced vertex attributes B. multibuffering means that to set color, one must set color 3 times over 3 frames C. we can't set those things until the render component gets its meshpool, and for perf reasons we cache mesh creation so that doesn't happen until RenderScene() is called.
    //void SetInstancedVertexAttribute(const unsigned int attributeName, const float& value);

    template <typename AttributeType>
    void SetInstancedVertexAttribute(const unsigned int attributeName, const AttributeType& value) = delete;
    template <>
    void SetInstancedVertexAttribute<float>(const unsigned int attributeName, const float& value);
    template <>
    void SetInstancedVertexAttribute<glm::vec2>(const unsigned int attributeName, const glm::vec2& value);
    template <>
    void SetInstancedVertexAttribute<glm::vec3>(const unsigned int attributeName, const glm::vec3& value);
    template <>
    void SetInstancedVertexAttribute<glm::vec4>(const unsigned int attributeName, const glm::vec4& value);
    template <>
    void SetInstancedVertexAttribute<glm::mat3x3>(const unsigned int attributeName, const glm::mat3x3& value);
    template <>
    void SetInstancedVertexAttribute<glm::mat4x4>(const unsigned int attributeName, const glm::mat4x4& value);
    

    // Equivalent to SetInstancedVertexAttribute() with the correct args
    void SetColor(const glm::vec4& color) {SetInstancedVertexAttribute(MeshVertexFormat::COLOR_ATTRIBUTE_NAME, color);}

    // Equivalent to SetInstancedVertexAttribute() with the correct args
    void SetTextureZ(const float textureZ) {SetInstancedVertexAttribute(MeshVertexFormat::TEXTURE_Z_ATTRIBUTE_NAME, textureZ);}

private:

    // TODO: getters for color, textureZ, etc. Should have a pointer that leads to that data so it doesn't get interleaved with the more hotly used data
    // glm::vec4 color;
    // float textureZ;

    // not const because object pool, don't actually change this
    //const unsigned int meshId;

    Meshpool::DrawHandle drawHandle;

    // -1 before being initialized
    int meshpoolId;

    friend class GraphicsEngine;

    // mesh.cpp needs to access mesh location sorry
    friend class Mesh;
    
    //private constructor to enforce usage of object pool
    //friend class ComponentPool<RenderComponent>;
    friend class RenderComponentNoFO;
    
};

// Identical to a RenderComponent in every way, except that it doesn't use floating origin. Not a bool flag on a normal render component bc i like premature optimization.
class RenderComponentNoFO: public RenderComponent {
public:
    // idk if we even call this when making one
    RenderComponentNoFO(unsigned int meshId, unsigned int materialId);
    RenderComponentNoFO(const RenderComponentNoFO&) = delete;
    // TODO: implicit conversion to normal rendercomponent because they do the exact same things?
    
private:

    //private constructor to enforce usage of object pool
    //friend class ComponentPool<RenderComponentNoFO>;
    //RenderComponentNoFO();

    // exists to let RenderComponentNoFO avoid type safety.
    // void* should be ptr to component pool
    //void SetPool(void* p) {
        //pool = (ComponentPool<RenderComponent>*)p;
    //}
};

static_assert(sizeof(RenderComponent) == sizeof(RenderComponentNoFO), "These classes need the exact same memory layout or i'll be sad.\n");
// this assertion fails but i'm gonna pretend it doesn't
// it's not undefined behaviour if you don't get caught
//static_assert(std::is_layout_compatible_v<RenderComponentNoFO, RenderComponent> == true, "These classes need the exact same memory layout or i'll be sad.\n");

