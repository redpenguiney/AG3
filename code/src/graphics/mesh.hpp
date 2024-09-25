#pragma once
// #include "pch/external_pch.h"
#include "glm/ext.hpp"
//#include "tinyobjloader/tiny_obj_loader.h"
#include "GL/glew.h"
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
#include "animation.hpp"
#include "texture_atlas.hpp"
#include "mesh_provider.hpp"

class Texture;

class Mesh;
class Material;
class ShaderProgram;
class PhysicsMesh;

// TODO: UNDO THIS CLASS'S EXISTENCE
class MeshGlobals {
    
public:
    // calls LOADED_MESHES.clear(), used to get around dependency of Mesh destructor on GE destructor 
    //void PurgeLoaded();

    
    static MeshGlobals& Get();

private:
    std::atomic<unsigned int> LAST_MESH_ID = {1}; // used to give each mesh a unique uuid
    std::unordered_map<unsigned int, std::shared_ptr<Mesh>> LOADED_MESHES; 
    //tinyobj::ObjReader OBJ_LOADER;
    friend class Mesh;

    unsigned int LAST_MATERIAL_ID = 1; // used to give each material a unique uuid TODO: why this one not atomic but the others are
    std::unordered_map<unsigned int,  std::shared_ptr<Material>> MATERIALS;
    friend class Material;

    
    //friend class BaseShaderProgram;
    //friend class ShaderProgram;
    //friend class ComputeShaderProgram;

    // std::unordered_map<unsigned int, std::shared_ptr<PhysicsMesh>> LOADED_PHYS_MESHES;

    // tuple<meshId, compressionLevel, convexDecomposition> 
    std::unordered_map<std::tuple<unsigned int, unsigned int, bool >, std::shared_ptr<PhysicsMesh>, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> MESHES_TO_PHYS_MESHES; 
    friend class PhysicsMesh;
    
    MeshGlobals();
    ~MeshGlobals();
};









// sets vertices/indices to contain the right stuff, not normalized. Doesn't actually make a mesh, sorry for dumb name.
// Defined in text_mesh.cpp
void TextMeshFromText(std::string text, const Texture &font, const TextMeshCreateParams& params, const MeshVertexFormat& vertexFormat, std::vector<GLfloat>& vertices, std::vector<GLuint>& indices);

// TODO: MAKE DYNAMIC MESHES UNUSABLE WHILE THEY ARE BEING MODIFIED
class Mesh: public std::enable_shared_from_this<Mesh> {
    public:
    const bool dynamic; // if a mesh is not dynamic, it saves memory and performance. If it is dynamic, you can modify the mesh using Start/StopModifying().

    // meshes have an id so that the engine can easily determine that two objects use the same mesh and instance them
    // It is absolutely critical that no two meshes have the same id, even if these meshes do not have overlapping lifetimes. Once a mesh gets an id, no other mesh gets that id, ever.
    const int meshId; 
    const std::vector<GLfloat>& vertices = meshVertices; // read only access to vertices
    const std::vector<GLuint>& indices = meshIndices; // read only access to indices
    const std::optional<std::vector<Animation>>& animations = meshAnimations;
    const std::optional<std::vector<Bone>>& bones = meshBones;

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
    
    ~Mesh();

    // Updates all objects using this mesh after you changed the mesh via StartModifying().
    // Must only be called after a call to StartModifying().
    void StopModifying(bool normalizeSize);

    // Replaces all vertices/indices with those given by the provider. Equivalent to setting vertices/indices using MeshProvider::GetMesh() with StartModifying() + StopModifying().
    // Mesh must be dynamic. 
    // Mesh vertex positions will be normalized after calling this if normalizeSize == true.
    void Remesh(const MeshProvider& provider, bool normalizeSize = true);

    // returns a square for gui meshes
    static std::shared_ptr<Mesh> Square();

    static bool IsValidForGameObject(unsigned int meshId);
    static std::shared_ptr<Mesh>& Get(unsigned int meshId);

    // literally don't know where this function went it vanished. anyways,
    // unloads the mesh with the given meshId, freeing its memory (once all other references to the mesh are destroyed). 
    // you CAN unload a mesh while objects are using it without any issues - a copy of that mesh is still on the gpu (TODO is that still true?).
    // you only need to call this function if you are (like for procedural terrain) dynamically loading new meshes
    static void Unload(int meshId);

    // verts must be organized in accordance with the given meshVertexFormat.
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the screen quad or skybox mesh) for internal usage
    //static std::shared_ptr<Mesh> FromVertices(const std::vector<GLfloat>& verts, const std::vector<GLuint> &indies, const MeshCreateParams& params);

    // creates a smooth terrain mesh from the given signed distance function using dual contouring, between p1 and p2 with the given resolution. 
    // The distance function must be continuous, and return distance values between -1 and 1 (TODO CONFIRM)
    // If a texture atlas is provided, it will use it for UVs and/or vertex colors if the format supports them.
    // Will also generate normals, tangents, etc. if requested by the format.
    // Mesh will be designed to be of dimensions size.
    // Returns optional because if the distance function finds the requested volume to be completely solid/empty, obviously no mesh should be generated.
    // fixVertexCenters puts all vertices in the center of each cell and disregards the normal function. resulting in blocky terrain.
    // TODO: doubles?
    //static std::optional<std::shared_ptr<Mesh>> FromVoxels(
    //    const MeshCreateParams& params, 
    //    glm::vec3 p1, 
    //    glm::vec3 p2,
    //    float resolution,
    //    std::function<float(glm::vec3)> distanceFunction, 
    //    std::optional<const TextureAtlas*> atlas = std::nullopt, // sorry its a pointer, should be a reference but it doesn't like optionals with references
    //    bool fixVertexCenters = false
    //);
 
    // Creates a mesh using the given MeshProvider.
    // if a mesh is not dynamic, it saves memory and performance. If it is dynamic, you can modify the mesh using Start/StopModifying().
    static std::shared_ptr<Mesh> New(const MeshProvider& provider, bool dynamic = false);

    // only accepts OBJ files.
    // File should just contain one object. 
    // TODO: DEPRECATED DO SOMETHING
    // TODO: materials
    // TODO: MTL support should be easy
    // TODO: caching mesh creation will be invaluable so that if same args are used, will return ptr to existing. If people want a copy instead of the same mesh, special arg? or specific Clone() method?
    // meshTransparency will be the initial alpha value of every vertex color, because obj files only support RGB (and also they don't REALLY support RGB).
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // textureZ is used as a starter value for every vertex's textureZ if it isn't instanced
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the screen quad or skybox mesh) for internal usage
    // static std::shared_ptr<Mesh> FromFile(const std::string& path, const MeshCreateParams& params = MeshCreateParams::Default());
    
    // Accepts most files (whatever assimp takes)
    // Creates meshes/materials for whatever the file needs (TODO: cache to avoid duplicate mats/meshes), and then returns those.
    // Each tuple contains a mesh, its material (WARNING: or nullptr if the mesh doesn't have a material), the textureZ, and an offset from the origin (so that a scene with many objects can be reassembled)
    // TODO: offset currently unimplemented bc kinda complicated
    static std::vector<std::tuple<std::shared_ptr<Mesh>, std::shared_ptr<Material>, float, glm::vec3>> MultiFromFile(
        const std::string& path, const MeshCreateParams& params = MeshCreateParams::Default()
    );

    // Creates a mesh for use in text/gui.
    // Modify the mesh with TextMeshFromText() to actually set text and what not.
    // Note: Disregards certain params. TODO be slightly more specific
    //static std::shared_ptr<Mesh> Text(const MeshCreateParams& params = MeshCreateParams::DefaultGui());

    private:

    // so PhysicsMesh can access physicsUsers
    friend class PhysicsMesh;
    // A list of physics meshes made using this mesh. We need to store this so that when the mesh is modified (assuming it's dynamic), it can update the physics mesh too.
    std::vector <PhysicsMesh*> physicsUsers;

    // true if the mesh was created using Mesh::FromText(). If that's the case, TODO WHAT WAS SUPPOSED TO FINISH THIS COMMENT LOL
    const bool wasCreatedFromText;

    // params.vertexFormat must have value
    Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, const MeshCreateParams& params, bool dynamic = false, bool fromText = false, std::optional<std::vector<Bone>> bones = std::nullopt, std::optional<std::vector<Animation>> anims = std::nullopt, const unsigned int rootBoneIndex = 0);

    // scale vertex positions into range -0.5 to 0.5 and calculate originalSize
    void NormalizePositions();

    std::vector<GLfloat> meshVertices;
    std::vector<GLuint> meshIndices;

    std::optional<std::vector<Bone>> meshBones;
    std::optional<std::vector<Animation>> meshAnimations; // TODO: allow empty
    const unsigned int rootBoneId; // index into meshBones of root bone (usually the spine for humanoids); undefined value if no bones
};