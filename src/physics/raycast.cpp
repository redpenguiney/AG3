#include <cassert>
#include <cmath>
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

    // std::cout << "Test of t is " << t << "\n";

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

// TODO: max distance for perf?
RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction) {
    auto possible_colliding = SpatialAccelerationStructure::Get().Query(origin, direction);
    // std::printf("Ray might be hitting ");
    // for (auto & collider: possible_colliding) {
    //     std::cout << collider->GetGameObject()->name << " ";
    // }
    // //std::printf("\n");
    //std::printf("muy guy %f %f %f\n", direction.x, direction.y, direction.z);

    // for every convex piece of every object's mesh, there are two hit triangles, and one of them is discarded through backface culling.
    // we'll then look thru the hit triangles/their objects to figure out which one the ray hit first.
    std::vector<RaycastResult> hitTriangles;
    
    for (auto & comp: possible_colliding) {
        auto& mesh = comp->physicsMesh;
        auto obj = comp->GetGameObject();
        assert(obj != nullptr);
        //std::cout << "Could be colliding with " << obj->name << ".\n";
        auto modelMatrix = obj->transformComponent->GetPhysicsModelMatrix();

        for (auto & convexMesh: mesh->meshes) {
            const unsigned int triCount = convexMesh.vertices.size()/9;
            //std::printf("There are %u triangles to test.\n", triCount);

            for (unsigned int i = 0; i < triCount; i++) {
                glm::dvec3 trianglePoints[3];
                for (unsigned int j = 0; j < 3; j++) {
                    auto vertexIndex = (i * 9) + (j * 3);
                    trianglePoints[j] = (modelMatrix * glm::dvec4(glm::vec4(convexMesh.vertices[vertexIndex], convexMesh.vertices[vertexIndex + 1], convexMesh.vertices[vertexIndex + 2], 1))).xyz();
                }

                auto normal = GetTriangleNormal(trianglePoints[0], trianglePoints[1], trianglePoints[2]);
                //std::printf("Triangle has points %f %f %f, %f %f %f, and %f %f %f, the normal is %f %f %f\n.", trianglePoints[0][0], trianglePoints[0][1], trianglePoints[0][2], trianglePoints[1][0], trianglePoints[1][1], trianglePoints[1][2], trianglePoints[2][0], trianglePoints[2][1], trianglePoints[2][2], normal.x, normal.y, normal.z);
                
                // backface culling so that we hit the right triangle
                if (glm::dot(normal, direction) < 0) { // works according to https://en.wikipedia.org/wiki/Back-face_culling
                    //std::cout << "Passed backface culling.\n";

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
        double closestDistance = INFINITY;
        RaycastResult bestResult;
        for (auto & result: hitTriangles) {
            std::cout << "Considering object at " << result.hitObject << " named " << result.hitObject->name << "\n";
            double distance = abs((result.hitPoint - origin).x) + abs((result.hitPoint - origin).y) + abs((result.hitPoint - origin).z);
            if (distance < closestDistance) {
                closestDistance = distance;
                bestResult = result;
            }

        }
        return bestResult;
    }
    
}