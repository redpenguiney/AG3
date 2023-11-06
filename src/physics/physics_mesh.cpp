#include "physics_mesh.hpp"
#include "../graphics/mesh.hpp"
#include <cassert>
#include <iostream>
#include <vector>

std::shared_ptr<PhysicsMesh> PhysicsMesh::New(std::shared_ptr<Mesh> &mesh, float simplifyThreshold, bool convexDecomposition) {
    if (MESHES_TO_PHYS_MESHES.count(mesh->meshId)) { // TODO: once we have simplifyThreshold and convexDecomposition, we need to make sure that matches up too
        return MESHES_TO_PHYS_MESHES[mesh->meshId];
    }
    else {
        auto ptr = std::shared_ptr<PhysicsMesh>(new PhysicsMesh(mesh));
        LOADED_PHYS_MESHES[ptr->physMeshId] = ptr;
        MESHES_TO_PHYS_MESHES[mesh->meshId] = ptr;
        return ptr;
    } 
}

// Returns a vector of ConvexMesh objects for a PhysicsMesh from the given Mesh. 
// TODO: convex decomposition

std::vector<PhysicsMesh::ConvexMesh> me_when_i_so_i_but_then_i_so_i(std::shared_ptr<Mesh>& mesh) {
    std::vector<float> verts;
    
    for (auto & i: mesh->indices) {
        auto vertexIndex = i * mesh->vertexSize/sizeof(GLfloat);
        verts.push_back(mesh->vertices[vertexIndex]);
        verts.push_back(mesh->vertices[vertexIndex + 1]);
        verts.push_back(mesh->vertices[vertexIndex + 2]);
    }

    // std::cout << "Created physics mesh.\n Vertices: ";
    // for (auto & v: verts) {
    //     std::cout << v << " ";
    // }
    // std::cout << "\n";
    return {PhysicsMesh::ConvexMesh {verts}};
}

PhysicsMesh::PhysicsMesh(std::shared_ptr<Mesh>& mesh): physMeshId(LAST_PHYS_MESH_ID++), meshes(me_when_i_so_i_but_then_i_so_i(mesh)) {

}

std::shared_ptr<PhysicsMesh>& PhysicsMesh::Get(unsigned int id) {
    assert(LOADED_PHYS_MESHES.count(id));
    return LOADED_PHYS_MESHES[id];
}