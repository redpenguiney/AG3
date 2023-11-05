#define GLM_FORCE_SWIZZLE
#include "raycast.hpp"
#include "spatial_acceleration_structure.hpp"
#include <cstdio>
#include <vector>

// copy pasted from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm#C++_implementation
// If the ray intersects the triangle, sets intersection point and returns true, else returns false
bool IsTriangleColliding(glm::dvec3 origin, glm::dvec3 direction, glm::dvec3 triVertex0, glm::dvec3 triVertex1, glm::dvec3 triVertex2, glm::dvec3& intersectionPoint) {
    const double EPSILON = 0.0000001;
    glm::dvec3 edge1, edge2, h, s, q;
    double a, f, u, v;
    edge1 = triVertex1 - triVertex0;
    edge2 = triVertex2 - triVertex0;
    h = glm::cross(direction, edge2);
    a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to this triangle.

    f = 1.0 / a;
    s = origin - triVertex0;
    u = f * glm::dot(s, h);

    if (u < 0.0 || u > 1.0)
        return false;

    q = glm::cross(s, edge1);
    v = f * glm::dot(direction, q);

    if (v < 0.0 || u + v > 1.0)
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    double t = f * glm::dot(edge2, q);

    if (t > EPSILON) // ray intersection
    {
        intersectionPoint = origin + direction * t;
        return true;
    }
    else {// This means that there is a line intersection but not a ray intersection.
        return false;
    }
}

glm::dvec3 GetTriangleNormal(glm::dvec3 triVertex0, glm::dvec3 triVertex1, glm::dvec3 triVertex2) {
    return glm::cross((triVertex1 - triVertex0), (triVertex2 - triVertex0));
}

// If ray did not hit anything, result.hitObject == nullptr.
RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction) {
    auto possible_colliding = SpatialAccelerationStructure::Get().Query(origin, direction);
    // std::printf("Ray might be hitting ");
    // for (auto & collider: possible_colliding) {
    //     std::cout << collider->GetGameObject()->name << " ";
    // }
    // //std::printf("\n");
    //std::printf("muy guy %f %f %f\n", direction.x, direction.y, direction.z);

    for (auto & comp: possible_colliding) {
        auto& mesh = comp->physicsMesh;
        auto obj = comp->GetGameObject();
        auto modelMatrix = obj->transformComponent->GetPhysicsModelMatrix();

        // for every convex piece of the mesh, there are two hit triangles, and one of them is discarded through backface culling.
        // we'll then sort the triangles to figure out which one the ray hit first.
        for (auto & convexMesh: mesh->meshes) {
            const unsigned int triCount = convexMesh.vertices.size()/9;
            //std::printf("There are %u triangles to test.\n", triCount);

            for (unsigned int i = 0; i < triCount; i++) {
                glm::dvec3 trianglePoints[3];
                for (unsigned int j = 0; j < 9; j++) {
                    trianglePoints[j/3][j % 3] = convexMesh.vertices[(i * 9) + j];
                }
                //std::printf("Triangle has points %f %f %f, %f %f %f, and %f %f %f\n.");
                glm::dvec3 intersectionPoint;
                auto normal = GetTriangleNormal(trianglePoints[0], trianglePoints[1], trianglePoints[2]);
                
                // backface culling so that we hit the right triangle
                if (glm::dot(normal, direction) < 0) { // works according to https://en.wikipedia.org/wiki/Back-face_culling
                    // TODO: IsTriangleColliding() might (?) independently calculate the normal which is waste of resources? compiler could probably optimize out that but idk
                    if (IsTriangleColliding(origin, direction, trianglePoints[0], trianglePoints[1], trianglePoints[2], intersectionPoint)) {
                        return RaycastResult {intersectionPoint, normal, obj};
                    }
                }
                
            }

            foundTriangle:;
        }

        
    }

    return RaycastResult {.hitObject = nullptr};
}