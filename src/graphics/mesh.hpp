#pragma once
// #include "pch/external_pch.h"
#include "glm/ext.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include "../../external_headers/GLEW/glew.h"
#include "utility/let_me_hash_a_tuple.hpp"
#include <optional>
#include <vector>
#include <set>
#include <unordered_map>
#include <array>
#include <tuple>
#include <memory>
#include <atomic>
#include <string>
#include "../debug/log.hpp"

class Texture;

class Mesh;
class Material;
class ShaderProgram;
class PhysicsMesh;

// TODO: UNDO THIS CLASS'S EXISTENCE
class MeshGlobals {
    
    public:
    
    static MeshGlobals& Get();

    private:
    std::atomic<unsigned int> LAST_MESH_ID = {1}; // used to give each mesh a unique uuid
    std::unordered_map<unsigned int, std::shared_ptr<Mesh>> LOADED_MESHES; 
    tinyobj::ObjReader OBJ_LOADER;
    friend class Mesh;

    unsigned int LAST_MATERIAL_ID = 1; // used to give each material a unique uuid TODO: why this one not atomic but the others are
    std::unordered_map<unsigned int,  std::shared_ptr<Material>> MATERIALS;
    friend class Material;

    GLuint LOADED_PROGRAM_ID = 0; // id of the currently loaded shader program
    std::unordered_map<unsigned int, std::shared_ptr<ShaderProgram>> LOADED_PROGRAMS; 
    friend class ShaderProgram;

    // std::unordered_map<unsigned int, std::shared_ptr<PhysicsMesh>> LOADED_PHYS_MESHES;

    // tuple<meshId, compressionLevel, convexDecomposition> 
    std::unordered_map<std::tuple<unsigned int, unsigned int, bool >, std::shared_ptr<PhysicsMesh>, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> MESHES_TO_PHYS_MESHES; 
    friend class PhysicsMesh;
    
    MeshGlobals();
    ~MeshGlobals();
};

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
    const static inline unsigned int N_ATTRIBUTES = 9;
    
    union { // this cursed union lets us either refer to vertex attributes by name, or iterate through them using the member vertexAttributes[]
        struct { // BRUH I HAD TO NAME THE STRUCT MID C++ IS MID
            std::optional<VertexAttribute> position;
            std::optional<VertexAttribute> textureUV; // TODO: when using colormap with fontmap, ability to use different uvs for each texture is needed? actually nvm i think just modifying shader and having 4-var uv is enough
            std::optional<VertexAttribute> textureZ; // texture z is seperate from the uvs because texture z is often instanced, but texture uv should never be instanced
            std::optional<VertexAttribute> color;
            std::optional<VertexAttribute> modelMatrix; // (will be all zeroes if not instanced, you must give it a value yourself which is TODO impossible) one 4x4 model matrix per thing being drawn, multiplying vertex positions by this puts them in the right position via rotating, scaling, and translating
            std::optional<VertexAttribute> normalMatrix; // (will be all zeroes if not instanced, you must give it a value yourself which is TODO impossible) normal matrix is like model matrix, but is 3x3 and for normals since it would be bad if a normal got scaled/translated  
            std::optional<VertexAttribute> normal; // lighting needs to know normals
            std::optional<VertexAttribute> tangent; // atangent vector is perpendicular to the normal and lies along the face of a triangle, used for normal/parallax mapping
            std::optional<VertexAttribute> arbitrary1;
        } attributes;

        std::optional<VertexAttribute> vertexAttributes[N_ATTRIBUTES];
    };

    // TODO: might be slow
    static unsigned int AttributeIndexFromAttributeName(unsigned int name) {
        switch (name) {
        case POS_ATTRIBUTE_NAME:
        return 0;
        case TEXTURE_UV_ATTRIBUTE_NAME:
        return 1;
        case TEXTURE_Z_ATTRIBUTE_NAME:
        return 2;
        case COLOR_ATTRIBUTE_NAME:
        return 3;
        case MODEL_MATRIX_ATTRIBUTE_NAME:
        return 4;
        case NORMAL_MATRIX_ATTRIBUTE_NAME:
        return 5;
        case NORMAL_ATTRIBUTE_NAME:
        return 6;
        case TANGENT_ATTRIBUTE_NAME:
        return 7;
        case ARBITRARY_ATTRIBUTE_1_NAME:
        return 8;
        default:
        DebugLogError("YOU FOOL, ", name, " IS NO VALID ATTRIBUTE NAME");
        abort();
        }
    }
    
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
    static inline const GLuint ARBITRARY_ATTRIBUTE_1_NAME = 13;

    // returns combined size in bytes of each non-instanced vertex attribute for one vertex
    unsigned int GetNonInstancedVertexSize() const;

    // returns combined size in bytes of each instanced vertex attribute for one vertex
    unsigned int GetInstancedVertexSize() const;

    // Returns a simple mesh vertex format that should work for normal people doing normal things in 3D.
    static MeshVertexFormat Default(bool instancedColor = true, bool instancedTextureZ = true);

    // Returns a simple mesh vertex format that should work for normal people doing normal things with GUI. Just XYZ UV.
    static MeshVertexFormat DefaultGui();

    // Takes a VAO and sets its noninstanced vertex attributes using VertexAttribPointer().
    // The VAO must ALREADY BE BOUND.
    void SetNonInstancedVaoVertexAttributes(GLuint& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const;

    // Takes a VAO and sets its instanced vertex attributes using VertexAttribPointer().
    // The VAO must ALREADY BE BOUND.
    void SetInstancedVaoVertexAttributes(GLuint& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const;
    void HandleAttribute(GLuint& vaoId, const std::optional<VertexAttribute>& attribute, const GLuint attributeName, bool justInstanced, unsigned int instancedSize, unsigned int nonInstancedSize) const;

    bool operator==(const MeshVertexFormat& other) const;
};

enum class HorizontalAlignMode {
    Left,
    Center,
    Right
};

enum class VerticalAlignMode{
    Top,
    Center,
    Bottom
};

struct MeshCreateParams {
    // TODO: "auto" option would be nice for vertex format
    const MeshVertexFormat meshVertexFormat = MeshVertexFormat::Default(); // todo: unconst?
    
    // default value of textureZ for the mesh's vertices, if that is a noninstanced vertex attribute
    float textureZ=-1.0;

    // default value of transaprency/alpha for the mesh's vertices, if color is a noninstanced 4-component vertex attribute
    float opacity=1.0;

    // meshpool will make room for this many objects to use this mesh (if you go over it's fine, but performance may be affected)
    // the memory cost of this is ~64 * expectedCount bytes if you ask for space you don't nee
    // default of 1024 should be fine in almost all cases
    // NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or TODO memory issues i should probably address at somepoint
    unsigned int expectedCount = 1024;

    // size should always be normalized for collisions/physics to work. Only set to false if you're making a weird mesh like the skybox or gui or something.
    // to actually change a rendercomponent's mesh's size, scale its transform component, using Mesh::originalSize if you want the mesh at its correct size.
    bool normalizeSize = true;

    // if a mesh is not dynamic, it saves memory and performance. If it is dynamic, you can modify the mesh using Start/StopModifying().
    bool dynamic = false;

    static MeshCreateParams Default();
    static MeshCreateParams DefaultGui();
};

struct TextMeshCreateParams {
    GLfloat leftMargin, rightMargin, topMargin, bottomMargin; // in pixels, 0 is the center of the ui text is being put on. top and bottom margin are only respected for top and bottom vertical alignment respectively.
    GLfloat lineHeightMultiplier; // 1 is single spaced, 2 is double spaced, etc.
    HorizontalAlignMode horizontalAlignMode;
    VerticalAlignMode verticalAlignMode;
    bool wrapText = true;
};

// sets vertices/indices to contain the right stuff, not normalized. Doesn't actually make a mesh, sorry for dumb name.
void TextMeshFromText(std::string text, const Texture &font, const TextMeshCreateParams& params, const MeshVertexFormat& vertexFormat, std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);

// TODO: MAKE DYNAMIC MESHES UNUSABLE WHILE THEY ARE BEING MODIFIED
class Mesh {
    public:
    const bool dynamic; // if a mesh is not dynamic, it saves memory and performance. If it is dynamic, you can modify the mesh using Start/StopModifying().

    const int meshId; // meshes have an id so that the engine can easily determine that two objects use the same mesh and instance them
    const std::vector<GLfloat>& vertices = meshVertices; // read only access to vertices
    const std::vector<GLuint>& indices = meshIndices; // read only access to indices

    const unsigned int instanceCount; // meshpool will make room for this many objects to use this mesh (if you go over it's fine, but performance may be affected)
                                      // the memory cost of this is ~64 * instanceCount bytes
                                      // def make it like a million for cubes and stuff, otherwise default of 1024 should be fine
                                      // NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or TODO memory issues i should probably address at somepoint
    
    glm::vec3 originalSize; // When loading a mesh, it is automatically scaled so all vertex positions are in the range -0.5 to 0.5. (this lets you and the physics engine easily know what the actual size of the object is) Set gameobject scale to this value to restore it to original size.
    
    
    const unsigned int nonInstancedVertexSize; // the size, in bytes, of a single vertex's noninstanced attributes.
    const unsigned int instancedVertexSize; // the size, in bytes, of a single vertex's instanced attributes. 

    const MeshVertexFormat vertexFormat;

    // returns a reference to vertices/indices, allowing you to mess with the mesh as you see fit.
    // you must immediately use this reference to modify the mesh, and then call StopModifying() to apply the changes. Don't hold onto this reference.
    // Also, completely ignores the RenderableMesh class, but you shouldn't care about that.
    // NOTE: EITHER ALL POSITIONS YOU GIVE MUST BE IN THE RANGE [-0.5, 0.5], OR YOU SHOULD OVERWRITE ALL POSITIONS TO BE IN MODEL SPACE. NO EXCEPTIONS.
    // Mesh must be dynamic.
    std::pair<std::vector<GLfloat>&, std::vector<GLuint>&> StartModifying();
    
    // Updates all objects using this mesh after you changed the mesh via StartModifying().
    // Must only be called after a call to StartModifying().
    void StopModifying(bool normalizeSize);

    // returns a square for gui meshes
    static std::shared_ptr<Mesh> Square();

    static bool IsValidForGameObject(unsigned int meshId);
    static std::shared_ptr<Mesh>& Get(unsigned int meshId);

    // literally don't know where this function went it vanished. anyways,
    // unloads the mesh with the given meshId, freeing its memory.
    // you CAN unload a mesh while objects are using it without any issues - a copy of that mesh is still on the gpu.
    // you only need to call this function if you are (like for procedural terrain) dynamically loading new meshes
    static void Unload(int meshId);

    // verts must be organized in accordance with the given meshVertexFormat.
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the screen quad or skybox mesh) for internal usage
    static std::shared_ptr<Mesh> FromVertices(const std::vector<GLfloat>& verts, const std::vector<GLuint> &indies, const MeshCreateParams& params);

    // only accepts OBJ files.
    // File should just contain one object. 
    // TODO: materials
    // TODO: MTL support should be easy
    // TODO: caching mesh creation will be invaluable so that if same args are used, will return ptr to existing. If people want a copy instead of the same mesh, special arg? or specific Clone() method?
    // meshTransparency will be the initial alpha value of every vertex color, because obj files only support RGB (and also they don't REALLY support RGB).
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // textureZ is used as a starter value for every vertex's textureZ if it isn't instanced
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the screen quad or skybox mesh) for internal usage
    static std::shared_ptr<Mesh> FromFile(const std::string& path, const MeshCreateParams& params = MeshCreateParams::Default());
    
    // Accepts most files (whatever assimp takes)
    // Creates meshes/materials for whatever the file needs (TODO: cache to avoid duplicate mats/meshes), and then returns those.
    // Each tuple contains a mesh, its material (WARNING: or nullptr if the mesh doesn't have a material), the textureZ, and an offset from the origin (so that a scene with many objects can be reassembled)
    // TODO: offset currently unimplemented
    static std::vector<std::tuple<std::shared_ptr<Mesh>, std::shared_ptr<Material>, float, glm::vec3>> MultiFromFile(const std::string& path);

    // Creates a mesh for use in text/gui.
    // Modify the mesh with TextMeshFromText() to actually set text and what not.
    // Note: Disregards certain params. TODO be slightly more specific
    static std::shared_ptr<Mesh> Text(const MeshCreateParams& params = MeshCreateParams::DefaultGui());

    private:

    // true if the mesh was created using Mesh::FromText(). If that's the case, TODO WHAT WAS SUPPOSED TO FINISH THIS COMMENT LOL
    const bool wasCreatedFromText;

    Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, const MeshCreateParams& params, bool fromText = false);

    // scale vertex positions into range -0.5 to 0.5 and calculate originalSize
    void NormalizePositions();

    std::vector<GLfloat> meshVertices;
    std::vector<GLuint> meshIndices;
};