#include "gjk.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>
#include <iostream>
#include "../../external_headers/GLM/gtx/string_cast.hpp"

// the GJK support function. returns farthest vertex along a directional vector
glm::dvec3 findFarthestVertexOnObject(const glm::dvec3& directionInWorldSpace, const TransformComponent& transform, const SpatialAccelerationStructure::ColliderComponent& collider) {
    // the physics mesh's vertices are obviously in model space, so we must put search direction in model space.
    // this way, to find the farthest vertex on a 10000-vertex mesh we don't need to do 10000 vertex transformations
    // TODO: this is still O(n vertices) complexity

    //std::cout << "Normal matrix is " << glm::to_string(transform.GetNormalMatrix()) << "\n";
    auto worldToModel = glm::inverse(transform.GetNormalMatrix());
    auto directionInModelSpace = glm::vec3(glm::normalize(worldToModel * glm::vec4(directionInWorldSpace.x, directionInWorldSpace.y, directionInWorldSpace.z, 1)));

    float farthestDistance = -FLT_MAX;
    glm::vec3 farthestVertex;

    //TODO: concave support
    for (unsigned int i = 0; i < collider.physicsMesh->meshes.at(0).vertices.size()/3; i++) {
        const glm::vec3& vertex = glm::make_vec3(&collider.physicsMesh->meshes.at(0).vertices.at(i * 3));
        auto dp = glm::dot(vertex, (directionInModelSpace)); // TODO: do we normalize vertex before taking dot product? i don't think so???
        if (dp >= farthestDistance) {
            farthestDistance = dp;
            farthestVertex = vertex;
        }
    }

    //std::cout << "Model matrix is " << glm::to_string(transform.GetPhysicsModelMatrix()) << "\n";
    //std::cout << "Support: farthest vertex in direction " << glm::to_string(directionInModelSpace) << " is " << glm::to_string(farthestVertex) << "\n";

    // put returned point in world space
    const auto& modelToWorld = transform.GetPhysicsModelMatrix();
    auto farthestVertexInWorldSpace = glm::dvec3(modelToWorld * glm::dvec4(farthestVertex.x, farthestVertex.y, farthestVertex.z, 1));
    //std::cout << "\tIn world space that's " << glm::to_string(farthestVertexInWorldSpace) << "\n";
    return farthestVertexInWorldSpace;
}

// helper function for GJK, look inside GJK() for explanation of purpose
void LineCase(std::vector<glm::dvec3>& simplex, glm::dvec3& searchDirection) {
    auto & a = simplex[0];
    auto & b = simplex[1];

    auto ab = b - a;
    auto ao = -a; // a to origin

    // https://www.youtube.com/watch?app=desktop&v=MDusDn8oTSE 5:43 has a nice picture to illustrate this
    // in this case, the 2 points of the simplex describe a plane that contains the origin if the vector between the 2 points is within 90 degrees of the vector from one of the points to to the origin
    if (glm::dot(ab, ao) >= 0) {
        // make search direction go towards origin again
        
        //std::cout << "\tPassed line case.\n";
        searchDirection = glm::cross(glm::cross(ab, ao), ab);
    }
    else { // if the condition failed, the 1st point is between 2nd point and the origin and thus the 2nd point won't help determine whether simplex contains the origin
        //std::cout << "\tFailed line case.\n";
        simplex = {a}; 
        searchDirection = ao;
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
            //std::cout << "\t Failed triangle case 1.\n";
            searchDirection = glm::cross(glm::cross(ac, ao), ac);
        }
        else {
            simplex = {a, b}; // TODO: ???
            LineCase(simplex, searchDirection);
            //std::cout << "\t Failed triangle case 2.\n";
        }
    }
    else {
        if (glm::dot(glm::cross(ab, abc), ao) >= 0) {
            simplex = {a, b}; // TODO: ???
            //std::cout << "\t Failed triangle case 3.\n";
            LineCase(simplex, searchDirection);
        }
        else {
            if (glm::dot(abc, ao) >= 0) {
                searchDirection = abc;
                //std::cout << "\t Succeeded triangle case 1.\n";
            }
            else {
                simplex = {a, c, b};
                searchDirection = -abc;
                //std::cout << "\t Succeeded triangle case 2.\n";
            }
        }
    }
    searchDirection = glm::normalize(searchDirection);
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

    // These are the normals of the 3 triangles in the tetrahedron. (the 4th triangle normal, bcd, is not needed because the triangle case checked it)
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

glm::dvec3 NewSimplexPoint(
    const glm::dvec3& searchDirection,
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) {
    return findFarthestVertexOnObject(searchDirection, transform2, collider2) 
    - findFarthestVertexOnObject(-searchDirection, transform1, collider1);
}

// Used by EPA to test if the reverse of an edge already exists in the list and if so, remove it.
// I don't really know why it needs that tho.
void AddIfUniqueEdge(std::vector<std::pair<unsigned int, unsigned int>>& edges, const std::vector<unsigned int>& faces, unsigned int a, unsigned int b) {
	auto reverse = std::find(           
		edges.begin(),                           
		edges.end(),                             
		std::make_pair(faces[b], faces[a]) 
	);
 
	if (reverse != edges.end()) {
		edges.erase(reverse);
	}
 
	else {
		edges.emplace_back(faces[a], faces[b]);
	}
}

// Used by EPA to get (take a guess) face normals.
// Returns vector of pair {normal, distance to face} and index of the closest normal.
std::pair<std::vector<std::pair<glm::dvec3, double>>, unsigned int> GetFaceNormals(const std::vector<glm::dvec3>& polytope, const std::vector<unsigned int>& faces) {
	std::vector<std::pair<glm::dvec3, double>> normals;
	size_t minTriangle = 0;
	double  minDistance = FLT_MAX;

	for (size_t i = 0; i < faces.size(); i += 3) {
		glm::dvec3 a = polytope[faces[i    ]];
		glm::dvec3 b = polytope[faces[i + 1]];
		glm::dvec3 c = polytope[faces[i + 2]];

		glm::dvec3 normal = glm::normalize(glm::cross(b - a, c - a));
		double distance = dot(normal, a);

		if (distance < 0) {
			normal   *= -1;
			distance *= -1;
		}

		normals.push_back({normal, distance});

		if (distance < minDistance) {
			minTriangle = i / 3;
			minDistance = distance;
		}
	}

	return { normals, minTriangle };
}
 
// this article actually does a really good job of explaining the GJK algorithm.
// https://cse442-17f.github.io/Gilbert-Johnson-Keerthi-Distance-Algorithm/
//  EPA algorithm used to get collision normals/contact points explained here: https://winter.dev/articles/epa-algorithm
// TODO: concave support
std::optional<CollisionInfo> IsColliding(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) 
{
    std::vector<glm::dvec3> simplex;

    // Search direction is in WORLD space.
    glm::dvec3 searchDirection = glm::normalize(glm::dvec3 {1, 1, 1}); // arbitrary starting direction
    //std::printf("Initial search direction %f %f %f\n", searchDirection.x, searchDirection.y, searchDirection.z);

    // add starting point to simplex
        // Subtracting findFarthestVertex(direction) from findFarthestVertex(-direction) gives a point on the Minoski difference of the two objects.
        // If the minowski difference of the 2 objects contains the origin, there is a point where the two positions subtracted from each other = 0, meaning the two objects are colliding.
        // Again, check the link above if you don't get it.
        // The simplex is just (in 3d) 4 points in the minoski difference that will be enough to determine whether the objects are colliding.
    simplex.push_back(NewSimplexPoint(searchDirection, transform1, collider1, transform2, collider2));
    // make new search direction go from simplex towards origin
    searchDirection = glm::normalize(-simplex.back());

    while (true) {

        // get new point for simplex
        auto newSimplexPoint = NewSimplexPoint(searchDirection, transform1, collider1, transform2, collider2);
        //std::printf("Going in direction %f %f %f\n", searchDirection.x, searchDirection.y, searchDirection.z);
        

        // this is the farthest point in this direction, so if it didn't get past the origin, then origin is gonna be outside the minoski difference meaning no collision.
        if (glm::dot(newSimplexPoint, searchDirection) <= 0) {
            //std::cout << "GJK failed with " << simplex.size() << " vertices.\n";
            // while (true) {}
            return std::nullopt;
        }

        // add point to simplex
        // we gotta insert at beginning because simplex order matters
        simplex.insert(simplex.begin(), newSimplexPoint);

        // the code for this next part depends on # of points in the current simplex, but its basically:
        // 1. see if origin intersects simplex
        // 2. if it does and we have 4 points, collision detected!
        // 3. if it does but not 4 points yet, compute new search direction and go back to beginning of loop to find more points
        // 4. if it doesn't, we have an unneccesary point in the simplex, reduce the simplex to closest/most relevant stuff to origin by doing some dot/cross product stuff, compute new search direction, and go back to beginning of loop to find more points
        switch (simplex.size()) {
            case 2: 
            //std::cout << "Executing line case.\n";
            LineCase(simplex, searchDirection); 
            break;
            case 3:
            //std::cout << "Executing triangle case.\n";
            TriangleCase(simplex, searchDirection);
            break;
            case 4:
            //std::cout << "Executing tetrahedron case.\n";
            if (TetrahedronCase(simplex, searchDirection)) { // this function is not void like the others, returns true if collision confirmed
                goto collisionFound;
            }
            break;
            default:
            std::cout << "GJK: WHAT\n";
            abort();
            break;
        }

        
    }

    collisionFound:; // goto uses this when collision found because c++ still doesn't let you label loops
    //std::cout << "Collision identified. Doing EPA.\n";

    // ///////////////////////////////////////////

    // Now that we know there's a collision, get collision normals, hit points, penetration depth, etc. using EPA algorithm
 
    // Simplex is no longer a simplex and is just a convex polytope (3d polygon) made from (more than 4) points on the Minkoski difference.
    auto & polytope = simplex;

    // To find the normal, we must progressively expand the simplex, which neccesitates knowing the faces of the simplex so that we can calculate proper normals
    std::vector<unsigned int> faces = {
		0, 1, 2,
		0, 3, 1,
		0, 2, 3,
		1, 3, 2
	};

    // vector<pair of normal + distance>, minFace = index to face with min distance
	auto [normals, minFace] = GetFaceNormals(polytope, faces);

    glm::dvec3 minNormal;
	float minDistance = FLT_MAX;
	
	while (minDistance == FLT_MAX) {
		minNormal   = normals[minFace].first;
		minDistance = normals[minFace].second;
 
		glm::dvec3 support = NewSimplexPoint(minNormal, transform1, collider1, transform2, collider2);
		double sDistance = glm::dot(minNormal, support);
 
		if (abs(sDistance - minDistance) > 0.001f) {
			minDistance = FLT_MAX;
            std::vector<std::pair<unsigned int, unsigned int>> uniqueEdges;

			for (unsigned int i = 0; i < normals.size(); i++) {
				if (glm::dot(normals[i].first, support) >= 0) {
					unsigned int f = i * 3;

					AddIfUniqueEdge(uniqueEdges, faces, f,     f + 1);
					AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
					AddIfUniqueEdge(uniqueEdges, faces, f + 2, f    );

					faces[f + 2] = faces.back(); faces.pop_back();
					faces[f + 1] = faces.back(); faces.pop_back();
					faces[f    ] = faces.back(); faces.pop_back();

					normals[i] = normals.back(); // pop-erase
					normals.pop_back();

					i--;
				}
			}

            std::vector<unsigned int> newFaces;
			for (auto [edgeIndex1, edgeIndex2] : uniqueEdges) {
				newFaces.push_back(edgeIndex1);
				newFaces.push_back(edgeIndex2);
				newFaces.push_back(polytope.size());
			}
			 
			polytope.push_back(support);

			auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

            float oldMinDistance = FLT_MAX;
			for (unsigned int i = 0; i < normals.size(); i++) {
				if (normals[i].second < oldMinDistance) {
					oldMinDistance = normals[i].second;
					minFace = i;
				}
			}
 
			if (newNormals[newMinFace].second < oldMinDistance) {
				minFace = newMinFace + normals.size();
			}
 
			faces  .insert(faces  .end(), newFaces  .begin(), newFaces  .end());
			normals.insert(normals.end(), newNormals.begin(), newNormals.end());
		}
	}

    assert((minNormal != glm::dvec3(0,0,0)));

    return CollisionInfo({
        .collisionNormal = minNormal,
        .hitPoints = {},
        .penetrationDepth = minDistance + 0.0001f // TODO 
    });
}