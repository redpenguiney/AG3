#pragma once
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

// TODO: this uses objects' Mesh instead of a simplified physics mesh 
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
        auto& obj = comp->GetGameObject();
        auto& mesh = Mesh::Get(obj->renderComponent->meshId);
        auto modelMatrix = obj->transformComponent->GetPhysicsModelMatrix();
        
        // test every triangle of the mesh against the ray, if any of them hit we win
        const unsigned int triCount = mesh->indices.size()/3;
        //std::printf("There are %u triangles to test.\n", triCount);

        const unsigned int floatsPerVertex = (sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + ((!mesh->instancedColor) ? sizeof(glm::vec4) : 0) + ((!mesh->instancedTextureZ) ? sizeof(GLfloat) : 0))/sizeof(GLfloat);
        for (unsigned int i = 0; i < triCount; i++) {
            glm::dvec3 trianglePoints[3];
            for (unsigned int j = 0; j < 3; j++) {
                int vertexIndex = mesh->indices[(i * 3) + j] * floatsPerVertex;
                glm::dvec4 point = glm::dvec4(mesh->vertices.at(vertexIndex), mesh->vertices.at(vertexIndex + 1), mesh->vertices.at(vertexIndex + 2), 1);
                point = modelMatrix * point;
                //std::printf("After %f %f %f %f\n", point.x, point.y, point.z, point.w);
                trianglePoints[j] = point.xyz();
            }
            //std::printf("Triangle has points %f %f %f, %f %f %f, and %f %f %f\n.");
            glm::dvec3 intersectionPoint;
            if (IsTriangleColliding(origin, direction, trianglePoints[0], trianglePoints[1], trianglePoints[2], intersectionPoint)) {
                return RaycastResult {intersectionPoint, GetTriangleNormal(trianglePoints[0], trianglePoints[1], trianglePoints[2]), obj};
            }
        }
    }

    return RaycastResult {.hitObject = nullptr};
}