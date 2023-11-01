#include "physics_mesh.hpp"

std::shared_ptr<PhysicsMesh> PhysicsMesh::New(std::shared_ptr<Mesh> &mesh, float simplifyThreshold, bool convexDecomposition) {
    auto ptr = std::shared_ptr<PhysicsMesh>(new PhysicsMesh);
    LOADED_PHYS_MESHES[ptr->physMeshId] = ptr;
    return ptr;
}

PhysicsMesh::PhysicsMesh(): physMeshId(LAST_PHYS_MESH_ID++) {

}