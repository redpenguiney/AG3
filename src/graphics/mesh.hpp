#pragma once
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/GLEW/glew.h"
#include <optional>
#include <vector>
#include <set>
#include <unordered_map>
#include <array>
#include <tuple>
#include <memory>
#include <atomic>
#include <string>
#include "../../external_headers/tinyobjloader/tiny_obj_loader.h"

// Something a vertex has; color, position, normal, etc.
struct VertexAttribute {
    // the offset, in bytes, of the vertex attribute.
    unsigned short offset;

    // the number of floats in each vertex attribute. (position would be 3 in a 3D situation, for example).
    // Must be greater than 0 and <= 4, or == 9 (for a 3x3 matrix) or == 12 or 16 (for 4x3 and 4x4 matrices).
    unsigned short nFloats;

    // whether this vertex attribute is for every vertex, or for every renderComponent. (so you could either give each vertex a different color, or just store one color for each object)
    bool instanced;

    bool operator==(const VertexAttribute& other) const = default;
};  

// Describes which vertex attributes a mesh has, which of them are instanced, and in what order they are in.
struct MeshVertexFormat {
    const static inline unsigned int N_ATTRIBUTES = 8;
    
    union { // this cursed union lets us either refer to vertex attributes by name, or iterate through them using the member vertexAttributes[]
        struct { // BRUH I HAD TO NAME THE STRUCT MID C++ IS MID
            std::optional<VertexAttribute> position;
            std::optional<VertexAttribute> textureUV;
            std::optional<VertexAttribute> textureZ; // texture z is seperate from the uvs because texture z is often instanced, but texture uv should never be instanced
            std::optional<VertexAttribute> color;
            std::optional<VertexAttribute> modelMatrix; // (will be all zeroes if not instanced, you must give it a value yourself which is TODO impossible) one 4x4 model matrix per thing being drawn, multiplying vertex positions by this puts them in the right position via rotating, scaling, and translating
            std::optional<VertexAttribute> normalMatrix; // (will be all zeroes if not instanced, you must give it a value yourself which is TODO impossible) normal matrix is like model matrix, but is 3x3 and for normals since it would be bad if a normal got scaled/translated  
            std::optional<VertexAttribute> normal; // lighting needs to know normals
            std::optional<VertexAttribute> tangent; // atangent vector is perpendicular to the normal and lies along the face of a triangle, used for normal/parallax mapping
        } attributes;

        std::optional<VertexAttribute> vertexAttributes[N_ATTRIBUTES];
    };
    
    static inline const GLuint POS_ATTRIBUTE_NAME = 0;
    static inline const GLuint COLOR_ATTRIBUTE_NAME = 1;
    static inline const GLuint TEXTURE_UV_ATTRIBUTE_NAME = 2;
    static inline const GLuint TEXTURE_Z_ATTRIBUTE_NAME = 3;
    static inline const GLuint NORMAL_ATTRIBUTE_NAME = 4;
    static inline const GLuint TANGENT_ATTRIBUTE_NAME = 5; 
    static inline const GLuint MODEL_MATRIX_ATTRIBUTE_NAME = 6; 
    /// model matrix takes up slots 7, 8, and 9 too because only one vec4 per attribute
    static inline const GLuint NORMAL_MATRIX_ATTRIBUTE_NAME = 10; 
    // normal matrix takes up slots 11 and 12 too for the same reason

    // returns combined size in bytes of each non-instanced vertex attribute for one vertex
    unsigned int GetNonInstancedVertexSize() const;

    // returns combined size in bytes of each instanced vertex attribute for one vertex
    unsigned int GetInstancedVertexSize() const;

    static MeshVertexFormat Default(bool instancedColor = true, bool instancedTextureZ = true);

    // Takes a VAO and sets its noninstanced vertex attributes using VertexAttribPointer().
    // The VAO must ALREADY BE BOUND.
    void SetNonInstancedVaoVertexAttributes(GLuint& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const;

    // Takes a VAO and sets its instanced vertex attributes using VertexAttribPointer().
    // The VAO must ALREADY BE BOUND.
    void SetInstancedVaoVertexAttributes(GLuint& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const;
    void HandleAttribute(GLuint& vaoId, const std::optional<VertexAttribute>& attribute, const GLuint attributeName, bool justInstanced, unsigned int instancedSize, unsigned int nonInstancedSize) const;

    bool operator==(const MeshVertexFormat& other) const;
};



// TODO: MODIFY MESH
class Mesh {
    public:
    
    const int meshId; // meshes have an id so that the engine can easily determine that two objects use the same mesh and instance them
    const std::vector<GLfloat> vertices;
    const std::vector<GLuint> indices; 

    const unsigned int instanceCount; // meshpool will make room for this many objects to use this mesh (if you go over it's fine, but performance may be affected)
                                      // the memory cost of this is ~64 * instanceCount bytes
                                      // def make it like a million for cubes and stuff, otherwise default of 1024 should be fine
                                      // NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or it's gonna cost you in memory
    
    const glm::vec3 originalSize; // When loading a mesh from file, it is automatically scaled so all vertex positions are in the range -0.5 to 0.5. (this lets you and the physics engine easily know what the actual size of the object is) Set gameobject scale to this value to restore it to original size.
    
    
    const unsigned int nonInstancedVertexSize; // the size, in bytes, of a single vertex's noninstanced attributes.
    const unsigned int instancedVertexSize; // the size, in bytes, of a single vertex's instanced attributes. 

    const MeshVertexFormat vertexFormat;
    

    static std::shared_ptr<Mesh>& Get(unsigned int meshId);

    // verts must be organized in accordance with the given meshVertexFormat.
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the screen quad or skybox mesh) for internal usage
    // TODO: how to pass verts by ref while still normalizing size? prob not worth trying to
    static std::shared_ptr<Mesh> FromVertices(std::vector<GLfloat> verts, const std::vector<GLuint> &indies, const MeshVertexFormat& meshVertexFormat, unsigned int expectedCount=1024, bool normalizeSize = true);

    // only accepts OBJ files.
    // File should just contain one object. 
    // TODO: materials
    // TODO: MTL support should be easy
    // meshTransparency will be the initial alpha value of every vertex color, because obj files only support RGB (and also they don't REALLY support RGA).
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // textureZ is used as a starter value for every vertex's textureZ if it isn't instanced
        // TODO: you can never actually change textureZ, or anything about a mesh for that matter. Need to fix that!
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the screen quad or skybox mesh) for internal usage
    static std::shared_ptr<Mesh> FromFile(const std::string& path, const MeshVertexFormat& meshVertexFormat, float textureZ=-1.0, unsigned int transparency=1.0, unsigned int expectedCount = 1024, bool normalizeSize = true);
    static void Unload(int meshId);

    private:
    Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, const MeshVertexFormat& meshVertexFormat, unsigned int expectedCount, glm::vec3 originalSize);
    inline static std::atomic<unsigned int> LAST_MESH_ID = {1};
    inline static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> LOADED_MESHES; 
    inline static tinyobj::ObjReader OBJ_LOADER;

    // helper function for SetVaoVertexAttributes()

};