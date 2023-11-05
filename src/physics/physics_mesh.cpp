#include "physics_mesh.hpp"
#include "../graphics/mesh.hpp"
#include <cassert>

std::shared_ptr<PhysicsMesh> PhysicsMesh::New(std::shared_ptr<Mesh> &mesh, float simplifyThreshold, bool convexDecomposition) {
    auto ptr = std::shared_ptr<PhysicsMesh>(new PhysicsMesh(mesh));
    LOADED_PHYS_MESHES[ptr->physMeshId] = ptr;
    return ptr;
}

PhysicsMesh::PhysicsMesh(std::shared_ptr<Mesh>& mesh): physMeshId(LAST_PHYS_MESH_ID++), meshes({ConvexMesh {mesh->vertices}}) {

}

std::shared_ptr<PhysicsMesh>& PhysicsMesh::Get(unsigned int id) {
    assert(LOADED_PHYS_MESHES.count(id));
    return LOADED_PHYS_MESHES[id];
}