#include "debug/assert.hpp"
#include <cmath>
#define GLM_FORCE_SWIZZLE
#include "raycast.hpp"
#include "spatial_acceleration_structure.hpp"
#include <cstdio>
#include <vector>
#include "utility/triangle_intersection.hpp"
#include "glm/gtx/string_cast.hpp"

glm::dvec3 GetTriangleNormal(glm::dvec3 triVertex0, glm::dvec3 triVertex1, glm::dvec3 triVertex2) {
    return glm::normalize(glm::cross((triVertex1 - triVertex0), (triVertex2 - triVertex0)));
}

// TODO: max distance for perf?
// TODO: IGNORES ANIMATION, FIX
RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction) {
    auto possible_colliding = SpatialAccelerationStructure::Get().Query(origin, direction);
    // std::printf("Ray might be hitting ");
    // for (auto & collider: possible_colliding) {
    //     std::cout << collider->GetGameObject()->name << " ";
    // }
    // std::printf("\n");
    //std::printf("muy guy %f %f %f\n", direction.x, direction.y, direction.z);

    // for every convex piece of every object's mesh, there are two hit triangles, and one of them is discarded through backface culling.
    // we'll then look thru the hit triangles/their objects to figure out which one the ray hit first.
    std::vector<RaycastResult> hitTriangles;
    
    for (auto & comp: possible_colliding) {
        auto& mesh = comp->physicsMesh;
        auto obj = comp->GetGameObject();
        Assert(obj != nullptr);
        // std::cout << "Could be colliding with " << obj->name << ".\n";
        auto modelMatrix = obj->Get<TransformComponent>()->GetPhysicsModelMatrix();

        for (auto & convexMesh: mesh->meshes) {
            for (auto & triangle: convexMesh.triangles) {

                // need to put vertices in world space to raycast against
                glm::dvec3 trianglePoints[3];
                for (unsigned int j = 0; j < 3; j++) {
                    trianglePoints[j] = (modelMatrix * glm::dvec4(triangle[j], 1)).xyz();
                }

                auto normal = GetTriangleNormal(trianglePoints[0], trianglePoints[1], trianglePoints[2]);
                
                
                // TODO rework physics_mesh.cpp so that triangles have clockwise winding; in the meantime we have to check normals because sometimes they backwards
                // TODO i thought i already did?
                if (glm::dot(normal, trianglePoints[0] - obj->Get<TransformComponent>()->Position()) < 0) {
                    normal *= -1;
                }

                // backface culling so that we hit the right triangle
                if (glm::dot(normal, direction) < 0) { // works according to https://en.wikipedia.org/wiki/Back-face_culling
                    //std::cout << "Passed backface culling.\n";
                    // std::printf("Triangle has points %f %f %f, %f %f %f, and %f %f %f, the normal is %f %f %f\n.", trianglePoints[0][0], trianglePoints[0][1], trianglePoints[0][2], trianglePoints[1][0], trianglePoints[1][1], trianglePoints[1][2], trianglePoints[2][0], trianglePoints[2][1], trianglePoints[2][2], normal.x, normal.y, normal.z);
                    glm::dvec3 intersectionPoint;

                    // TODO: IsTriangleColliding() might (?) independently calculate the normal which is waste of resources? compiler could probably optimize out that but idk        
                    if (IsTriangleColliding(origin, direction, trianglePoints[0], trianglePoints[1], trianglePoints[2], intersectionPoint)) {
                        //std::cout << "Ray intersects object at " << obj << " named " << obj->name << "\n";
                        hitTriangles.push_back({.hitPoint = intersectionPoint, .hitNormal = normal, .hitObject = obj});
                        goto foundTriangle;
                    }
                }
                
            }

            foundTriangle:;
        }

        
    }

    if (hitTriangles.size() == 0) { // If there were no hit triangles then yeah

        return RaycastResult {.hitObject = nullptr};
    }
    else { // else go through them to decide which triangle was hit first and return that one
        // closest hit point is first hit triangle
        double closestDistance = FLT_MAX;
        RaycastResult bestResult;
        for (auto & result: hitTriangles) {
            double distance = glm::length2(origin - result.hitPoint);
            // std::cout << "Considering object normal " << glm::to_string(result.hitNormal) << " named " << result.hitObject->name << " with distance " << distance << " vs closest " << closestDistance << "\n";
            if (distance < closestDistance) {
                closestDistance = distance;
                bestResult = result;
            }
        }

        return bestResult;
    }
    
}