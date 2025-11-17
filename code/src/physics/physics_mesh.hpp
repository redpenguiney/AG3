#include <array>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <utility>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

class Mesh;

class ConvexMesh {
public:
    // GJK's support function. 
    // Returns farthest point (in model space) lying on the surface of the convex mesh along a directional vector.
    virtual glm::vec3 FindFarthestPointOnObject(const glm::vec3& directionInModelSpace) = 0;
    virtual void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) = 0;
    virtual float GetVolume(glm::vec3 objectScale) = 0;
};

// Will probably crash out if you stretch out the sphere. Might not; who knows!
// Radius 0.5.
class SphereMesh : public ConvexMesh {
public:
    SphereMesh() {};

    glm::vec3 FindFarthestPointOnObject(const glm::vec3& directionInModelSpace) override { return directionInModelSpace * 0.5; }
    void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) override {
        Ia += density * 0.4f;
        Ib += density * 0.4f;
        Ic += density * 0.4f;
    }
    float GetVolume(glm::vec3 objectScale) override { return 4.0f / 3.0f * 3.1415926; }
};

class ConvexTriangleMesh: public ConvexMesh {
public:
    // TODO: convex decomposition, rn it will always return 1 mesh
    static std::vector<std::unique_ptr<ConvexMesh>> FromMesh(const std::shared_ptr<Mesh>& mesh, float simplifyThreshold, bool convexDecomposition);

    glm::vec3 FindFarthestPointOnObject(const glm::vec3& directionInModelSpace) override;
    float GetVolume(glm::vec3 objectScale) override;
    void AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) override;

private:
    ConvexTriangleMesh(
        const std::vector<std::array<glm::vec3, 3>> triangles,
        const std::vector<std::pair<glm::vec3, std::vector<glm::vec3>>> faces,
        const std::vector<std::pair<glm::vec3, glm::vec3>> edges,
        const std::vector<glm::vec3> vertices
    );

    // Raycasting wants triangles. TODO: MAKE THEM IN CLOCKWISE ORDER.
    const std::vector<std::array<glm::vec3, 3>> triangles;

    // Some collision algorithms (SAT contact points namely) rely on having polygonal faces instead of a bunch of triangles.
    // Pairs are <face normal, vector of vertex positions>.
    // Each face's vertices are sorted in clockwise order (painfully).
    const std::vector<std::pair<glm::vec3, std::vector<glm::vec3>>> faces;

    // SAT also needs edges annoyingly
    // each pair is two vertices
    const std::vector<std::pair<glm::vec3, glm::vec3>> edges;

    const std::vector<glm::vec3> vertices;
};

// A mesh used for collisions, since the meshes used for rendering are oftentimes too high fidelity to be performant, and/or are concave and thus require additional processing to be compatible with functions/algorithms that work only on convex shapes.
// Also calculates mass moment of inertia for arbitrary shapes.
class PhysicsMesh {
    public:

    ~PhysicsMesh();

    // static std::shared_ptr<PhysicsMesh>& Get(unsigned int id);

    // Creates a physics mesh based on the vertices of the (graphical) mesh.
    // If the given mesh is dynamic, then changes to the mesh will affect the physics mesh automatically.
    // If you call this function twice with the same arguments, you'll (hopefully) get 2 pointers to the same PhysicsMesh.
    // simplifyThreshold is TODO
    // convexDecomposition is TODO
    static std::shared_ptr<PhysicsMesh> New(std::shared_ptr<Mesh>& mesh, unsigned int simplifyThreshold = 0, bool convexDecomposition = false);
    
    // All spheres will share the same physics mesh lol
    static std::shared_ptr<PhysicsMesh> Sphere();
    
    // TODO: CONSTRUCT FROM VERTICES

    PhysicsMesh(const PhysicsMesh&) = delete;

    // Takes id of PhysicsMesh to unload as an argument.
    // Call if you aren't going to instantitate any more objects with this physics mesh and want to free up that memory.
    // It's okay to call this while colliders use this physics mesh, it's a shared_ptr so it won't be deleted until all those colliders die too
    // TODO: why this even a thing? just delete when references gone
    // static void Unload(unsigned int id);

    // const unsigned int physMeshId;

    // To allow for accurate, fast, and simple collisions with concave and convex objects, stores a vector of convex meshes.
    // Not const.
    std::vector<std::unique_ptr<ConvexMesh>> meshes; 

    glm::mat3x3 CalculateLocalMomentOfInertia(glm::vec3 objectScale, float objectMass);


    

    private:
    friend class Mesh;
    // overwrites "meshes" with new data
    void RefreshMesh();
    PhysicsMesh(std::shared_ptr<Mesh>& mesh);

    // Makes sphere mesh
    PhysicsMesh();

    // nullptr if physicsMesh was not created from a Mesh object; used to refresh the mesh.
    std::shared_ptr<Mesh> origin = nullptr;

    // the moi for this mesh when it is 1x1x1 size. TODO, might not actually be possible to get scaled moi from unscaled moi, but if it is, would make nice optimization.
    // const glm::mat3x3 baseMomentOfInertia;

};