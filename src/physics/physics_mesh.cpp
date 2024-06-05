#include "physics_mesh.hpp"
#include "../graphics/mesh.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../utility/let_me_hash_a_tuple.cpp"
#include "../../external_headers/GLM/gtx/string_cast.hpp"
#include "GLM/gtx/norm.hpp"



std::shared_ptr<PhysicsMesh> PhysicsMesh::New(std::shared_ptr<Mesh> &mesh, unsigned int simplifyThreshold, bool convexDecomposition) {
    auto tuple = std::make_tuple(mesh->meshId, simplifyThreshold, convexDecomposition);
    if (MeshGlobals::Get().MESHES_TO_PHYS_MESHES.count(tuple)) { // TODO: once we have simplifyThreshold and convexDecomposition, we need to make sure that matches up too
        return MeshGlobals::Get().MESHES_TO_PHYS_MESHES[tuple];
    }
    else {
        auto ptr = std::shared_ptr<PhysicsMesh>(new PhysicsMesh(mesh));
        // MeshGlobals::Get().LOADED_PHYS_MESHES[ptr->physMeshId] = ptr;
        MeshGlobals::Get().MESHES_TO_PHYS_MESHES[tuple] = ptr;
        return ptr;
    } 
}

// Returns a vector of ConvexMesh objects for a PhysicsMesh from the given Mesh. 
// TODO: convex decomposition
std::vector<PhysicsMesh::ConvexMesh> me_when_i_so_i_but_then_i_so_i(std::shared_ptr<Mesh>& mesh, float simplifyThreshold, bool convexDecomposition) {
    assert(!mesh->dynamic);
    assert(simplifyThreshold >= 1.0f);
    // DebugLogInfo("Here we go. ", mesh->meshId);

    // Graphics meshes contain extraneous data (UVs, colors, etc.) that isn't relevant to physics, so this function needs to get rid of that.
    // This function also needs to take triangles with same normal and put them in same polygon to fill faces, and get edges.
    std::vector<std::pair<glm::vec3, std::vector<glm::vec3>>> faces;
    std::vector<std::array<glm::vec3, 3>> triangles;
    std::vector<std::pair<glm::vec3, glm::vec3>> edges;

    assert(mesh->indices.size() % 3 == 0);
    for (auto it = mesh->indices.begin(); it != mesh->indices.end(); it+=3) {
        
        auto itCopy = it;

        auto vertexIndex1 = *(itCopy) * mesh->nonInstancedVertexSize/sizeof(GLfloat);
        auto vertexIndex2 =  *(itCopy + 1) * mesh->nonInstancedVertexSize/sizeof(GLfloat);
        auto vertexIndex3 =  *(itCopy + 2) * mesh->nonInstancedVertexSize/sizeof(GLfloat);

        auto offset = mesh->vertexFormat.attributes.position->offset/sizeof(GLfloat);
        glm::vec3 vertex1 = {mesh->vertices[vertexIndex1 + offset], mesh->vertices[vertexIndex1 + 1 + offset], mesh->vertices[vertexIndex1 + 2 + offset]};
        glm::vec3 vertex2 = {mesh->vertices[vertexIndex2 + offset], mesh->vertices[vertexIndex2 + 1 + offset], mesh->vertices[vertexIndex2 + 2 + offset]};
        glm::vec3 vertex3 = {mesh->vertices.at(vertexIndex3 + offset), mesh->vertices.at(vertexIndex3 + 1 + offset), mesh->vertices.at(vertexIndex3 + 2 + offset)};
        
        // DebugLogInfo("Vertices ", glm::to_string(vertex1), ",", glm::to_string(vertex2), ",", glm::to_string(vertex3));

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
                // DebugLogInfo("Found second face with normal ", glm::to_string(normal));

                bool containsVertex1 = false;
                bool containsVertex2 = false;
                bool containsVertex3 = false;
                for (auto & v: f.second) {
                    if (v == vertex1) {
                        // DebugLogInfo("Face already has ", glm::to_string(v));
                        containsVertex1 = true;
                    }
                    else if (v == vertex2) {
                        // DebugLogInfo("Face already has ", glm::to_string(v));
                        containsVertex2 = true;
                    }
                    else if (v == vertex3) {
                        // DebugLogInfo("Face already has ", glm::to_string(v));
                        containsVertex3 = true;
                    }
                }
                
                // DebugLogInfo("Before: f2 size = ", f.second.size());
                if (!containsVertex1) {
                    f.second.push_back(vertex1);
                }
                if (!containsVertex2) {
                    f.second.push_back(vertex2);
                }
                if (!containsVertex3) {
                    f.second.push_back(vertex3);
                }
                // DebugLogInfo("After: f2 size = ", f.second.size());

                break; // break exits the inner loop
            }
        }
        // if this is the first face we've found with this normal, create a new face.
        if (!foundFace) {
            faces.push_back({normal, {vertex1, vertex2, vertex3}});
        }

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

    // Then, we need to get edges of each face.
    // We don't do this when iterating through triangles since we don't want (for example) the diagonal of a square face to be treated as an edge, plus we might (???) want edges in clockwise order too.
    for (auto & f: faces) {
        for (unsigned int i = 0; i < f.second.size(); i++) {
            auto vertex1 = f.second[i];
            auto vertex2 = f.second[(i + 1 == f.second.size() ? 0 : i + 1)]; // make sure we get the edge between the last vertex and the first vertex of the face
            // check if this edge isn't already in the edges vector and if so add it
            bool foundEdge = false;
            for (auto & [a, b]: edges) {
                // the order of a and b doesn't matter so we gotta check both combinations
                if (a == vertex1 && b == vertex2) {foundEdge = true;}
                if (a == vertex2 && b == vertex1) {foundEdge = true;}
            }

            if (!foundEdge) {
                edges.emplace_back(vertex1, vertex2);
            }
        }
        
    }
    
    // std::cout << "Created physics mesh:\n";
    // for (auto & f: faces) {
    //     std::cout << "\tFace with normal " << glm::to_string(f.first) << " has vertices "; 
    //     for (auto & v: f.second) {
    //         std::cout << glm::to_string(v) << ", ";
    //     }
    //     std::cout << ".\n";
    // }
    // for (auto & e: edges) {
    //     std::cout << "\tEdge " << glm::to_string(e.first) << " to " << glm::to_string(e.second) << ".\n";
    // }
    return {PhysicsMesh::ConvexMesh {.triangles = triangles, .faces = faces, .edges = edges}};
}

PhysicsMesh::PhysicsMesh(std::shared_ptr<Mesh>& mesh): meshes(me_when_i_so_i_but_then_i_so_i(mesh, 1.0, false)) {

}

// std::shared_ptr<PhysicsMesh>& PhysicsMesh::Get(unsigned int id) {
    // assert(MeshGlobals::Get().LOADED_PHYS_MESHES.count(id));
    // return MeshGlobals::Get().LOADED_PHYS_MESHES[id];
// }

// helper function for CalculateLocalMomentOfInertia()
float TetrahedronVolume(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    // return abs(glm::determinant(glm::mat4x4(
    //     a.x, a.y, a.z, 1.0f, 
    //     b.x, b.y, b.z, 1.0f, 
    //     c.x, c.y, c.z, 1.0f, 
    //     d.x, d.y, d.z, 1.0f
    // ))/6.0f);
    return glm::dot(a, glm::cross(b, c))/6.0f;
}

// helper function for CalculateLocalMomentOfInertia(). i is matrix x, j is matrix y.
float ComputeInertiaProduct(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, unsigned int i, unsigned int j) {
    return (
        2.0 * p1[i] * p1[j] + p2[i] * p3[j] + p3[i] * p2[j] +
        2.0 * p2[i] * p2[j] + p1[i] * p3[j] + p3[i] * p1[j] +
        2.0 * p3[i] * p3[j] + p1[i] * p2[j] + p2[i] * p1[j]
    );
}

// helper function for CalculateLocalMomentOfInertia(). i is the matrix x-coordinate.
float ComputeInertiaMoment(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, unsigned int i) {
    return (
        pow(p1[i], 2.0f) + p2[i] * p3[i] +
        pow(p2[i], 2.0f) + p1[i] * p3[i] +
        pow(p3[i], 2.0f) + p1[i] * p2[i] 
    );      
}

glm::mat3x3 PhysicsMesh::CalculateLocalMomentOfInertia(glm::vec3 objectScale, float objectMass) {

    assert(objectMass > 0);
    assert(glm::length2(objectScale) > 0);

    // From http://number-none.com/blow/inertia/body_i.html and https://stackoverflow.com/questions/809832/how-can-i-compute-the-mass-and-moment-of-inertia-of-a-polyhedron 
    // and most especially https://www.youtube.com/watch?v=GYc99lMdcFE

    // calculate volume of object (needed to calculate density which is needed to calculate moi)
    float volume = 0.0;

    for (const auto & convexMesh: meshes) { // TODO: does this even work for concave things?
        for (const auto & triangle: convexMesh.triangles) {
            glm::vec3 p1 = triangle[0] * objectScale; 
            glm::vec3 p2 = triangle[1] * objectScale; 
            glm::vec3 p3 = triangle[2] * objectScale; 
                
            glm::vec3 triangleNormal = glm::cross(p2 - p1, p3 - p1);
            
            glm::vec3 triangleCentroid = (p1 + p2 + p3)/3.0f;
            
            // volume calc from https://math.stackexchange.com/questions/3616760/how-to-calculate-the-volume-of-tetrahedron-given-by-4-points
            float tetrahedronVolume = TetrahedronVolume(p1, p2, p3, {1.0, 1.0, 1.0});
            
            float dot = glm::dot(triangleNormal, triangleCentroid); // We're checking if the triangle normal points towards the origin.
            if (dot > 0.0) { // then the triangle's normal points away from the origin. 
                volume += tetrahedronVolume;
            }
            else { // then the triangle normal points towards the origin (because mesh has concave bits) and we gotta negate all the values it calculates.
                volume -= tetrahedronVolume;
            }

        }
    }

    assert(volume > 0);

    float density = objectMass / volume;

    glm::vec3 objectCenterOfMass = {0, 0, 0};
    float Ia = 0.0, Ib = 0.0, Ic = 0.0, Iap = 0.0, Ibp = 0.0, Icp = 0.0; // components of inertia tensor. i think.

    for (const auto & convexMesh: meshes) { // TODO: does this even work for concave things?
        for (const auto & triangle: convexMesh.triangles) {
            glm::vec3 p1 = triangle[0] * objectScale; 
            glm::vec3 p2 = triangle[1] * objectScale; 
            glm::vec3 p3 = triangle[2] * objectScale; 
            
            glm::vec3 triangleNormal = glm::cross(p2 - p1, p3 - p1);
            
            glm::vec3 triangleCentroid = (p1 + p2 + p3)/3.0f;
            glm::vec3 tetrahedronCenterOfMass = (p1 + p2 + p3)/4.0f; // not bothering to add the origin for obvious reasons
            
            // volume calc from https://math.stackexchange.com/questions/3616760/how-to-calculate-the-volume-of-tetrahedron-given-by-4-points
            float tetrahedronVolume = TetrahedronVolume(p1, p2, p3, {1.0, 1.0, 1.0});

            float tetrahedronMass = tetrahedronVolume * density;

            float dot = glm::dot(triangleNormal, triangleCentroid); // We're checking if the triangle normal points towards the origin.
            if (dot < 0.0) { // then the triangle's normal points towards the origin and must be negated. 
                tetrahedronMass *= -1;
                tetrahedronVolume *= -1;
            }

            objectCenterOfMass += tetrahedronCenterOfMass * tetrahedronMass; // we'll divide it to get actual average at the end

            // from 23:00 in the video i mentioned above
            Ia += 6.0f * tetrahedronVolume * (ComputeInertiaMoment(p1, p2, p3, 1) + ComputeInertiaMoment(p1, p2, p3, 2));
            Ib += 6.0f * tetrahedronVolume * (ComputeInertiaMoment(p1, p2, p3, 0) + ComputeInertiaMoment(p1, p2, p3, 2));
            Ic += 6.0f * tetrahedronVolume * (ComputeInertiaMoment(p1, p2, p3, 0) + ComputeInertiaMoment(p1, p2, p3, 1));
            Iap += 6.0 * tetrahedronVolume * ComputeInertiaProduct(p1, p2, p3, 1, 2);
            Ibp += 6.0 * tetrahedronVolume * ComputeInertiaProduct(p1, p2, p3, 0, 1);
            Icp += 6.0 * tetrahedronVolume * ComputeInertiaProduct(p1, p2, p3, 0, 2);
        }
    }

    objectCenterOfMass /= objectMass;
    Ia *= density/60.0f;
    Ib *= density/60.0f;
    Ic *= density/60.0f;
    Iap *= density/120.0f;
    Iap *= density/120.0f;
    Iap *= density/120.0f;

    // We just calculated inertia tensor with respect to the origin. Since all meshes are transformed to be centered on the origin, we're done here.
    glm::mat3x3 inertiaTensor {
        Ia, -Ibp, -Icp,
        -Ibp, Ib, -Iap,
        -Icp, -Iap, Ic
    };

    return inertiaTensor;
}