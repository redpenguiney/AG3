#include "gjk.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>
#include <iostream>

// the GJK support function. returns farthest vertex along a directional vector
glm::dvec3 findFarthestVertexOnObject(const glm::dvec3& directionInWorldSpace, const TransformComponent& transform, const SpatialAccelerationStructure::ColliderComponent& collider) {
    // the physics mesh's vertices are obviously in model space, so we must put search direction in model space.
    // this way, to find the farthest vertex on a 10000-vertex mesh we don't need to do 10000 vertex transformations
    // TODO: this is still O(n vertices) complexity

    auto worldToModel = glm::inverse(transform.GetNormalMatrix());
    auto direction = glm::normalize(directionInWorldSpace * worldToModel);

    float farthestDistance = -INFINITY;
    glm::vec3 farthestVertex;

    //TODO: concave support
    for (unsigned int i = 0; i < collider.physicsMesh->meshes.at(0).vertices.size()/3; i++) {
        const glm::vec3& vertex = glm::make_vec3(&collider.physicsMesh->meshes.at(0).vertices.at(i * 3));
        auto dp = glm::dot(vertex, direction); // TODO: do we normalize vertex before taking dot product? i don't think so???
        if (dp > farthestDistance) {
            farthestDistance = dp;
            farthestVertex = vertex;
        }
    }

    // put returned point in world space
    const auto& modelToWorld = transform.GetPhysicsModelMatrix();
    return glm::dvec3(glm::dvec4(farthestVertex.x, farthestVertex.y, farthestVertex.z, 1) * modelToWorld);
}

// helper function for GJK, look inside GJK() for explanation of purpose
void LineCase(std::vector<glm::dvec3>& simplex, glm::dvec3& searchDirection) {
    // https://www.youtube.com/watch?app=desktop&v=MDusDn8oTSE 5:43 has a nice picture to illustrate this
    // in this case, the 2 points of the simplex describe a plane that contains the origin if the vector between the 2 points is within 90 degrees of the vector from one of the points to to the origin
    if (glm::dot(simplex[1] - simplex[0], -simplex[0]) >= 0) {
        // make search direction go towards origin again
        searchDirection = glm::cross(glm::cross(simplex[1] - simplex[0], -simplex[0]), simplex[1] - simplex[0]);
    }
    else { // if the condition failed, the 1st point is between 2nd point and the origin and thus the 2nd point won't help determine whether simplex contains the origin
        simplex.erase(simplex.begin()); 
    }
}

// helper function for GJK, look inside GJK() for explanation of purpose
void TriangleCase(std::vector<glm::dvec3>& simplex, glm::dvec3& searchDirection) { 
    auto& a = simplex[0];
    auto& b = simplex[1];
    auto& c = simplex[2];

    auto ab = b - a;
    auto ac = c - a;
    auto ao = -a; // (a to origin)

    auto abc  = glm::cross(ab, ac); // normal of the plane defined by the 3 points of the simplex

    if (glm::dot(glm::cross(abc, ac), ao) >= 0) {
        if (glm::dot(ac, ao) >= 0) {
            simplex = {a, c};
            searchDirection = glm::cross(glm::cross(ac, ao), ac);
        }
        else {
            LineCase(simplex, searchDirection);
        }
    }
    else {
        if (glm::dot(glm::cross(ab, abc), ao) >= 0) {
            simplex = {a, b}; // TODO: ???
            LineCase(simplex, searchDirection);
        }
        else {
            if (glm::dot(abc, ao) >= 0) {
                searchDirection = abc;
            }
            else {
                simplex = {a, c, b};
                searchDirection = -abc;
            }
        }
    }

}

bool TetrahedronCase(std::vector<glm::dvec3>& simplex, glm::dvec3& searchDirection) {
    auto& a = simplex[0];
    auto& b = simplex[1];
    auto& c = simplex[2];
    auto& d = simplex[3];

    auto ab = b - a;
    auto ac = c - a;
    auto ad = d - a;
    auto ao = -a;

    // These are the normals of the 3 triangles in the tetrahedron. (the 4th triangle normal, bcd is not needed because the triangle case checked it)
    auto abc = glm::cross(ab, ac);
    auto acd = glm::cross(ac, ad);
    auto adb = glm::cross(ad, ab);

    // if it's on the inside of all 3 of these triangles, then collision detected.
    // if it's in front of a triangle's normal, remove the simplex point not included in that triangle, and search in front of that normal for a point.
    if (glm::dot(abc, ao) >= 0) {
        simplex = {a, b, c};
        TriangleCase(simplex, searchDirection);
        return false;
    }
    else if (glm::dot(acd, ao) >= 0) {
        simplex = {a, c, d};
        TriangleCase(simplex, searchDirection);
        return false;
    }
    else if (glm::dot(adb, ao) >= 0) {
        simplex = {a, d, b};
        TriangleCase(simplex, searchDirection);
        return false;
    }

    return true;
}
 
// this article actually does a really good job of explaining the GJK algorithm.
// https://cse442-17f.github.io/Gilbert-Johnson-Keerthi-Distance-Algorithm/
// TODO: concave support
std::optional<CollisionInfo> GJK(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) 
{
    std::vector<glm::dvec3> simplex;

    // Search direction is in WORLD space.
    glm::dvec3 searchDirection = {1, 0, 0}; // arbitrary starting direction

    // add starting point to simplex
        // Subtracting findFarthestVertex(direction) from findFarthestVertex(-direction) gives a point on the Minoski difference of the two objects.
        // If the minowski difference of the 2 objects contains the origin, there is a point where the two positions subtracted from each other = 0, meaning the two objects are colliding.
        // Again, check the link above if you don't get it.
        // The simplex is just (in 3d) 4 points in the minoski difference that will be enough to determine whether the objects are colliding.
    simplex.push_back(findFarthestVertexOnObject(searchDirection, transform1, collider1) - findFarthestVertexOnObject(-searchDirection, transform2, collider2));
    // make new search direction go from simplex towards origin
    searchDirection = -simplex.back();

    while (true) {

        // get new point for simplex
        auto newSimplexPoint = findFarthestVertexOnObject(searchDirection, transform1, collider1) - findFarthestVertexOnObject(-searchDirection, transform2, collider2);
        std::printf("Found point %f %f %f by going in direction %f %f %f\n", newSimplexPoint.x, newSimplexPoint.y, newSimplexPoint.z, searchDirection.x, searchDirection.y, searchDirection.z);

        // this is the farthest point in this direction, so if it didn't get past the origin, then origin is gonna be outside the minoski difference meaning no collision.
        if (glm::dot(newSimplexPoint, searchDirection) <= 0) {
            std::cout << "GJK failed with " << simplex.size() << " vertices.\n";
            return std::nullopt;
        }

        // add point to simplex
        // we gotta insert at beginning because simplex order matters
        simplex.insert(simplex.begin(), newSimplexPoint);

        // the code for this next part depends on # of points in the current simplex (and is also not understood by me), but its basically:
        // 1. see if origin intersects simplex
        // 2. if it does and we have 4 points, collision detected!
        // 3. if it does but not 4 points yet, go back to beginning of loop to find more points
        // 4. if it doesn't, we have an unneccesary point in the simplex, reduce the simplex to closest/most relevant stuff to origin
        switch (simplex.size()) {
            case 2: 
            std::cout << "Executing line case.\n";
            LineCase(simplex, searchDirection); 
            break;
            case 3:
            std::cout << "Executing triangle case.\n";
            TriangleCase(simplex, searchDirection);
            break;
            case 4:
            std::cout << "Executing tetrahedron case.\n";
            if (TetrahedronCase(simplex, searchDirection)) { // this function is not void like the others, returns true if collision confirmed
                goto collisionFound;
            }
            break;
            default:
            std::cout << "WHAT\n";
            abort();
            break;
        }

        
    }

    collisionFound:;
    return CollisionInfo();
}