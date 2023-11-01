#include <memory>
#include <atomic>
#include <unordered_map>
#include <vector>

class Mesh;

// A mesh used for collisions, since the meshes uses for rendering are too high fidelity to be performant oftentimes or are concave and require additional processing.
class PhysicsMesh {
    public:
    static std::shared_ptr<PhysicsMesh>& Get(unsigned int id);

    // Creates a physics mesh based on the vertices of the (graphical) mesh.
    // simplifyThreshold is TODO
    // convexDecomposition is TODO
    static std::shared_ptr<PhysicsMesh> New(std::shared_ptr<Mesh>& mesh, float simplifyThreshold = 0, bool convexDecomposition = true);

    // Takes id of PhysicsMesh to unload as an argument.
    // Call if you aren't going to instantitate any more objects with this physics mesh and want to free up that memory.
    // It's okay to call this while colliders use this physics mesh, it's a shared_ptr so it won't be deleted until all those colliders die too
    static void Unload(unsigned int id);

    const unsigned int physMeshId;

    struct ConvexMesh {
        std::vector<double> vertices; // TODO: could be float instead?
        std::vector<unsigned int> indices;
    };

    // To allow for accurate, fast, and simple collisions with concave and convex objects, stores a vector of convex meshes
    const std::vector<ConvexMesh> meshes; 

    private:
    PhysicsMesh();

    inline static std::unordered_map<unsigned int, std::shared_ptr<PhysicsMesh>> LOADED_PHYS_MESHES; 
    inline static std::atomic<unsigned int> LAST_PHYS_MESH_ID = {1};
};