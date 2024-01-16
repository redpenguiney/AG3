#include "physics_mesh.hpp"
#include "../graphics/mesh.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../utility/let_me_hash_a_tuple.cpp"

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
    // Graphics meshes contain extraneous data (normals, colors, etc.) that isn't relevant to physics, so this function needs to get rid of that.
    // This function also needs to take triangles with same normal and put them in same polygon to fill faces, and get edges.
    std::vector<std::pair<glm::vec3, std::vector<glm::vec3>>> faces;
    std::vector<std::array<glm::vec3, 3>> triangles;
    std::vector<std::pair<glm::vec3, glm::vec3>> edges;

    assert(mesh->indices.size() % 3 == 0);
    for (unsigned int i = 0; i < mesh->indices.size(); i += 3) {
        auto vertexIndex1 = i * mesh->vertexSize/sizeof(GLfloat);
        auto vertexIndex2 = (i + 1) * mesh->vertexSize/sizeof(GLfloat);
        auto vertexIndex3 = (i + 2) * mesh->vertexSize/sizeof(GLfloat);

        glm::vec3 vertex1 = {mesh->vertices[vertexIndex1], mesh->vertices[vertexIndex1 + 1], mesh->vertices[vertexIndex1 + 2]};
        glm::vec3 vertex2 = {mesh->vertices[vertexIndex2], mesh->vertices[vertexIndex2 + 1], mesh->vertices[vertexIndex2 + 2]};
        glm::vec3 vertex3 = {mesh->vertices[vertexIndex3], mesh->vertices[vertexIndex3 + 1], mesh->vertices[vertexIndex3 + 2]};
        
        auto normal = glm::cross(vertex1 - vertex2, vertex1 - vertex3);

        // make sure normal always points outward
        double distance = glm::dot(normal, vertex1);
		if (distance < 0) { // if dot product between center of model to vertex and the normal is < 0, normal is opposite direction of model to vertex and needs to be flipped
			normal   *= -1;
			distance *= -1;
		}

        // if we already started a face with this normal, add our nonduplicate vertices to it. 
        bool foundFace = false;
        for (auto & f: faces) {
            if (glm::dot(f.first, normal) > 0.99f) { // this dot product check lets the normals be very slightly different (TODO: change threshold? do we even want threshold?)
                foundFace = true;

                bool containsVertex1 = false;
                bool containsVertex2 = false;
                bool containsVertex3 = false;
                for (auto & v: f.second) {
                    if (v == vertex1) {
                        containsVertex1 = true;
                    }
                    else if (v == vertex2) {
                        containsVertex2 = true;
                    }
                    else if (v == vertex3) {
                        containsVertex3 = true;
                    }
                }
                
                if (!containsVertex1) {
                    f.second.push_back(vertex1);
                }
                if (!containsVertex2) {
                    f.second.push_back(vertex2);
                }
                if (!containsVertex3) {
                    f.second.push_back(vertex3);
                }

                break; // break exits the inner loop
            }
        }
        // if this is the first face we've found with this normal, create a new face.
        if (!foundFace) {
            faces.push_back({normal, {vertex1, vertex2, vertex3}});
        }

        // check if any of the edges of this triangle aren't in the edges vector, and if so add them.
        bool foundEdge12 = false;
        bool foundEdge13 = false;
        bool foundEdge23 = false;
        for (auto & [a, b]: edges) {
            // the order of a and b doesn't matter so we gotta check both combinations
            if (a == vertex1 && b == vertex2) {foundEdge12 = true;}
            if (a == vertex2 && b == vertex1) {foundEdge12 = true;}

            if (a == vertex1 && b == vertex1) {foundEdge13 = true;}
            if (a == vertex3 && b == vertex1) {foundEdge13 = true;}

            if (a == vertex2 && b == vertex3) {foundEdge23 = true;}
            if (a == vertex3 && b == vertex2) {foundEdge23 = true;}
        }

        if (!foundEdge12) {edges.emplace_back(vertex1, vertex2);}
        if (!foundEdge13) {edges.emplace_back(vertex1, vertex3);}
        if (!foundEdge23) {edges.emplace_back(vertex2, vertex3);}

        // and of course add triangle to triangles vector
        triangles.push_back({vertex1, vertex2, vertex3});
    }

    // Then, we need to sort each face's vertices so that they're in clockwise order, because SAT needs that too.
    for (auto & face: faces) {
        
        auto faceCenter = glm::vec3(0, 0, 0);
        for (auto & v: face.second) {
            faceCenter += v;
        }
        faceCenter /= face.second.size();

        const auto r = face.second.at(0) - faceCenter; // use an arbitrary vector as the twelve o’clock reference
        const auto p = cross(r, faceCenter);

        std::sort(face.second.begin(), face.second.end(), [&faceCenter, &p, &r](const glm::vec3 &v1, const glm::vec3&v2) {
            
            // stolen from https://stackoverflow.com/questions/47949485/sorting-a-list-of-3d-points-in-clockwise-order
            // the sort function should return true if v1 is clockwise from v2 around the center of the face.

            auto u1 = v1 - faceCenter;
            auto u2 = v2 - faceCenter;
            auto h1 = glm::dot(u1, p);
            auto h2 = glm::dot(u2, p);

            if (h2 <= 0 && h1 > 0) {
                return false;
            }
            else if( h1 <= 0 && h2 > 0) {
                return true;
            }
            else if (h1 == 0 && h2 == 0) {
                return (glm::dot(u1, r) > 0 && glm::dot(u2, r) < 0);
            }
            else {
                return (glm::dot(glm::cross(u1, u2), faceCenter) > 0);
            }
                    
        });
    }
    
    // std::cout << "Created physics mesh.\n Vertices: ";
    // for (auto & v: verts) {
    //     std::cout << v << " ";
    // }
    // std::cout << "\n";
    return {PhysicsMesh::ConvexMesh {.triangles = triangles, .faces = faces, .edges = edges}};
}

PhysicsMesh::PhysicsMesh(std::shared_ptr<Mesh>& mesh): physMeshId(LAST_PHYS_MESH_ID++), meshes(me_when_i_so_i_but_then_i_so_i(mesh)) {

}

std::shared_ptr<PhysicsMesh>& PhysicsMesh::Get(unsigned int id) {
    assert(LOADED_PHYS_MESHES.count(id));
    return LOADED_PHYS_MESHES[id];
}