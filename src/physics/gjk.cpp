#include "gjk.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "../utility/utility.hpp"
#include <optional>
#include <utility>
#include <vector>
#include <iostream>
#include "../../external_headers/GLM/gtx/string_cast.hpp"
#include "../gameobjects/component_registry.hpp"

// DEAR GOD. YOU DON'T KNOW. A MONTH AND A HALF WAS SPENT SUFFERING IN THIS FILE. I NEVER WANT TO TOUCH THIS AGAIN.
// DECEMBER TO FEBRUARY 2ND. FINALLY.
// feb 14 it wasn't enough apparently

// the GJK support function. returns farthest vertex (in world space) along a directional vector 
glm::dvec3 FindFarthestVertexOnObject(const glm::dvec3& directionInWorldSpace, const TransformComponent& transform, const SpatialAccelerationStructure::ColliderComponent& collider) {
    // the physics mesh's vertices are obviously in model space, so we must put search direction in model space.
    // this way, to find the farthest vertex on a 10000-vertex mesh we don't need to do 10000 vertex transformations
    // TODO: this is still O(n vertices) complexity

    assert(directionInWorldSpace.x != NAN);
    assert(directionInWorldSpace.y != NAN);
    assert(directionInWorldSpace.z != NAN);

    // std::cout << "Normal matrix is " << glm::to_string(transform.GetNormalMatrix()) << "\n";
    auto worldToModel = glm::inverse(transform.GetNormalMatrix());
    auto directionInModelSpace = glm::vec3(glm::normalize(worldToModel * glm::vec4(directionInWorldSpace.x, directionInWorldSpace.y, directionInWorldSpace.z, 1)));

    assert(directionInModelSpace.x != NAN);
    assert(directionInModelSpace.y != NAN);
    assert(directionInModelSpace.z != NAN);


    float farthestDistance = -FLT_MAX;
    glm::vec3 farthestVertex = {0, 0, 0};

    //TODO: concave support
    for (auto & face: collider.physicsMesh->meshes.at(0).faces) {
        for (auto & vertex: face.second) {
            auto dp = glm::dot(vertex, (directionInModelSpace)); // TODO: do we normalize vertex before taking dot product? i don't think so???
            if (dp >= farthestDistance) {
                farthestDistance = dp;
                farthestVertex = vertex;
            }
        } 
    }

    //// std::cout << "Model matrix is " << glm::to_string(transform.GetPhysicsModelMatrix()) << "\n";
    // std::cout << "Support: farthest vertex in direction " << glm::to_string(directionInModelSpace) << " is " << glm::to_string(farthestVertex) << "\n";

    // put returned point in world space
    const auto& modelToWorld = transform.GetPhysicsModelMatrix();
    auto farthestVertexInWorldSpace = glm::dvec3(modelToWorld * glm::dvec4(farthestVertex.x, farthestVertex.y, farthestVertex.z, 1));
    //// std::cout << "\tIn world space that's " << glm::to_string(farthestVertexInWorldSpace) << "\n";
    return farthestVertexInWorldSpace;
}

// helper function for GJK, look inside GJK() for explanation of purpose
void LineCase(std::vector<std::array<glm::dvec3, 3>>& simplex, glm::dvec3& searchDirection) {
    auto & a = simplex[0];
    auto & b = simplex[1];

    auto ab = b[0] - a[0];
    auto ao = -a[0]; // a to origin

    // https://www.youtube.com/watch?app=desktop&v=MDusDn8oTSE 5:43 has a nice picture to illustrate this
    // in this case, the 2 points of the simplex describe 2 parallel planes whose volume contain the origin if the vector between the 2 points is within 90 degrees of the vector from one of the points to to the origin
    if (glm::dot(ab, ao) >= 0) {
        // make search direction go towards origin again
        
        // std::cout << "\tPassed line case.\n";
        searchDirection = glm::normalize(glm::cross(glm::cross(ab, ao), ab));
    }
    else { // if the condition failed, the 1st point is between 2nd point and the origin and thus the 2nd point won't help determine whether simplex contains the origin
        // std::cout << "\tFailed line case.\n";
        simplex = {a}; 
        searchDirection = ao;
    }
}

// helper function for GJK, look inside GJK() for explanation of purpose
void TriangleCase(std::vector<std::array<glm::dvec3, 3>>& simplex, glm::dvec3& searchDirection) { 
    auto& a = simplex[0];
    auto& b = simplex[1];
    auto& c = simplex[2];

    auto ab = b[0] - a[0];
    auto ac = c[0] - a[0];
    auto ao = -a[0]; // (a to origin)

    auto abc  = glm::normalize(glm::cross(ab, ac)); // normal of the plane defined by the 3 points of the simplex

    if (glm::dot(glm::cross(abc, ac), ao) >= 0) {
        if (glm::dot(ac, ao) >= 0) {
            simplex = {a, c};
            //// std::cout << "\t Failed triangle case 1.\n";
            searchDirection = glm::normalize(glm::cross(glm::cross(ac, ao), ac));
        }
        else {
            simplex = {a, b}; // TODO: ???
            LineCase(simplex, searchDirection);
            //// std::cout << "\t Failed triangle case 2.\n";
        }
    }
    else {
        if (glm::dot(glm::cross(ab, abc), ao) >= 0) {
            simplex = {a, b}; // TODO: ???
            //// std::cout << "\t Failed triangle case 3.\n";
            LineCase(simplex, searchDirection);
        }
        else {
            if (glm::dot(abc, ao) >= 0) {
                searchDirection = abc;
                //// std::cout << "\t Succeeded triangle case 1.\n";
            }
            else {
                simplex = {a, c, b};
                searchDirection = -abc;
                //// std::cout << "\t Succeeded triangle case 2.\n";
            }
        }
    }
    searchDirection = glm::normalize(searchDirection);
}

bool TetrahedronCase(std::vector<std::array<glm::dvec3, 3>>& simplex, glm::dvec3& searchDirection) {
    auto& a = simplex[0];
    auto& b = simplex[1];
    auto& c = simplex[2];
    auto& d = simplex[3];

    auto ab = b[0] - a[0];
    auto ac = c[0] - a[0];
    auto ad = d[0] - a[0];
    auto ao = -a[0];

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

// returns the actual minkoski point, followed by the support points in world (?) space
// the actual support points in world space are needed to get contact points from EPA
std::array<glm::dvec3, 3> NewSimplexPoint(
    const glm::dvec3& searchDirection,
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) {

    auto a = FindFarthestVertexOnObject(searchDirection, transform1, collider1);
    auto b = FindFarthestVertexOnObject(-searchDirection, transform2, collider2);
    // std::cout << "SUPPORT: Farthest point in " << glm::to_string(searchDirection) << " is " << glm::to_string(a - b) << ".\n";
    return {a-b, a, b};
}

// Used by EPA to test if the reverse of an edge already exists in the list and if so, remove it, otherwise add the unreversed edge.
// I don't really know why it needs that tho.
void AddIfUniqueEdge(std::vector<std::pair<unsigned int, unsigned int>>& edges, const std::vector<unsigned int>& faces, unsigned int a, unsigned int b) {
    // std::cout << "Adding if unique edge, are currently " << edges.size() << "\n";
	auto reverse = std::find(           
		edges.begin(),                           
		edges.end(),                             
		std::make_pair(faces[b], faces[a]) 
	);
 
	if (reverse != edges.end()) {
        // std::cout << "eraser\n";
		edges.erase(reverse);
	}
 
	else {
        // std::cout << "emplacing, size was previously " << edges.size() << "\n";
		edges.emplace_back(faces[a], faces[b]);
        // std::cout << "now its " << edges.size() << "\n";
	}
}

// Used by EPA to get (take a guess) face normals.
// Returns vector of pair {normal, distance to face} and index of the closest normal.
std::pair<std::vector<std::pair<glm::dvec3, double>>, unsigned int> GetFaceNormals(
    const std::vector<std::array<glm::dvec3, 3>>& polytope, 
    const std::vector<unsigned int>& faces) 
    {
	std::vector<std::pair<glm::dvec3, double>> normals;
    assert(faces.size() > 0);
	size_t minTriangle = 0;
	double  minDistance = FLT_MAX;

    // std::cout << "There are " << faces.size() << " face indices.\n";
	for (size_t i = 0; i < faces.size(); i += 3) {
		auto& a = polytope[faces[i    ]];
		auto& b = polytope[faces[i + 1]];
		auto& c = polytope[faces[i + 2]];

		glm::dvec3 normal = glm::normalize(glm::cross(b[0] - a[0], c[0] - a[0]));
		double distance = glm::dot(normal, a[0]);

		if (distance < 0) {
			normal   *= -1;
			distance *= -1;
		}

        // std::cout << "Pushing back to normals.\n";
		normals.emplace_back(std::make_pair(normal, distance));

		if (distance < minDistance) {
			minTriangle = i / 3;
			minDistance = distance;
            // std::cout << "Min distance " << minDistance << " created by dot of " << glm::to_string(normal) << " and support " << glm::to_string(a[0]) << "\n";
		}
	}

    assert(normals.size() > 0);
	return { normals, minTriangle };
}

// used by FindContact()
double SignedDistanceToPlane(glm::dvec3 planeNormal, glm::dvec3 point, glm::dvec3 pointOnPlane) {
    return glm::dot(planeNormal, point - pointOnPlane);
}

struct SatFacesResult {
    glm::vec3 farthestNormal; // in world space
    double farthestDistance;
    const std::vector<glm::vec3>* farthestFace; // in model space
};

// Used by SAT algorithm in FindContactPoint().
// Does SAT by testing collider1's faces against collider 2's vertices.
SatFacesResult SatFaces(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) 
{
    double farthestDistance = -FLT_MAX;
    glm::vec3 farthestNormal(0, 0, 0); // in world space
    const std::vector<glm::vec3>* farthestFace = nullptr; // in model space

    for (auto & face1: collider1.physicsMesh->meshes.at(0).faces) {

        auto normalInWorldSpace = glm::normalize(transform1.GetNormalMatrix() * face1.first);

        // make sure normal faces out of collider 1 because somehow it wasn't???
        glm::dvec3 pointOnPlaneInWorldSpace = transform1.GetPhysicsModelMatrix() * glm::dvec4(face1.second.at(0), 1);
        if (glm::dot(glm::dvec3(normalInWorldSpace), pointOnPlaneInWorldSpace - transform1.Position()) < 0) {
            normalInWorldSpace *= -1;
            // std::cout << "bruh we had to switch???\n";
        }

        auto vertex2 = FindFarthestVertexOnObject(-normalInWorldSpace, transform2, collider2);
        double distance = SignedDistanceToPlane(normalInWorldSpace, vertex2, pointOnPlaneInWorldSpace);
        
        // they're colliding, so distance should be negative, so flip normal/distance if it's not
        // if (distance > 0) {
        //     distance *= -1;
        //     normalInWorldSpace *= -1;
        // }

        if (distance > farthestDistance) {
            // if (distance > 0) {
            //     std::cout << "\tDistance was too big, giving up.\n";
            //     farthestDistance = -FLT_MAX;
            //     break;
            // }
            // std::cout << "\tNormal " << glm::to_string(normalInWorldSpace) << " replaced normal " << glm::to_string(farthestNormal) << ", having distance " << distance << " which is less than " << farthestDistance << "\n";
            // std::cout << "\t\tYunno in model space that normal was " << glm::to_string(face1.first) << ".\n";
            // std::cout << "\t\tPoint on plane was " << glm::to_string(pointOnPlaneInWorldSpace) << "\n";
            // std::cout << "\t\tSupport point was " << glm::to_string(vertex2) << "\n";
            farthestDistance = distance;
            farthestFace = &face1.second;
            farthestNormal = normalInWorldSpace;
        }
        
        // std::cout << "d = " << distance << "\n";
        assert(distance <= 0.1);
    }

    assert(farthestFace != nullptr);
    return {farthestNormal, farthestDistance, farthestFace};
}

// Returns the contact points for a face-face collision.
// Each pair in the return value is <hitPoint, penetrationDepth>.
// referenceNormal is normal of refereneceFace IN WORLD SPACE.
// referenceFace is (IN MODEL SPACE) the face that had the least penetration, and referenceTransform is the transform of the collider that contains referenceFace.
std::vector<std::pair<glm::dvec3, double>> ClipFaceContactPoints(
    const glm::vec3 referenceNormal,
    const std::vector<glm::vec3>* referenceFace, 
    const TransformComponent& referenceTransform, 
    const TransformComponent& otherTransform,
    const SpatialAccelerationStructure::ColliderComponent& otherCollider
) {
    // std::cout << "Clipping face contact points: reference face has normal " << glm::to_string(referenceNormal) << ".\n";
    // find the face on otherCollider with a normal closest to -normal.
    const std::vector<glm::vec3>* incidentFace = nullptr;
    float smallestDot = FLT_MAX;
    for (auto & face: otherCollider.physicsMesh->meshes.at(0).faces) {
        auto dot = glm::dot(otherTransform.GetNormalMatrix() * face.first, referenceNormal);
        if (dot < smallestDot) {
            incidentFace = &face.second;
            // std::cout << "Set incident face to thing with normal " << glm::to_string(otherTransform.GetNormalMatrix() * face.first) << ".\n";
            smallestDot = dot;
        }
    }

    assert(incidentFace != nullptr); // mostly to shut up compiler 
    assert(referenceFace != nullptr); // this one we actually had an issue

    // Get reference face and incident face in world space
    std::vector<glm::dvec3> referenceFaceInWorldSpace;
    for (auto & v: *referenceFace) {
        referenceFaceInWorldSpace.push_back(referenceTransform.GetPhysicsModelMatrix() * glm::dvec4(v, 1.0));
    }

    std::vector<glm::dvec3> incidentFaceInWorldSpace;
    for (auto & v: *incidentFace) {
        
        incidentFaceInWorldSpace.push_back(otherTransform.GetPhysicsModelMatrix() * glm::dvec4(v, 1.0));
    }

    // for (auto & v: incidentFaceInWorldSpace) {
        // std::cout << "Incident face has vertex " << glm::to_string(v) << "\n";
    // }

    // Get side planes for reference face.
    // Pairs are <normal, pointOnPlane>
    std::vector<std::pair<glm::dvec3, glm::dvec3>> sidePlanes;
    for (unsigned int i = 0; i < referenceFaceInWorldSpace.size(); i++) {
        // get edge
        auto v1 = referenceFaceInWorldSpace[i];
        auto v2 = referenceFaceInWorldSpace[i + 1 == referenceFaceInWorldSpace.size() ? 0 : i + 1];

        // get side plane normal by taking cross product of edge and reference face normal
        glm::vec3 planeNormal = glm::normalize(glm::cross(glm::vec3(v2 - v1), referenceNormal));
        

        // flip normal if needed so normal faces away from the face
        // TODO: needed??
        if (glm::dot(glm::dvec3(planeNormal), v1 - referenceTransform.Position()) < 0) {
            planeNormal *= -1;
            // std::cout << "UH OH FLIPPING\n";
        }
        // std::cout << "Side plane has normal " << glm::to_string(planeNormal) << " and point " << glm::to_string(v1) << ".\n"; 
        sidePlanes.emplace_back(planeNormal, v1);
    }

    // Clip the incident face against those side planes of the reference face, using the Sutherland-Hodgman algorithm 
    // (https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm)
    // This will get the contact area.
    std::vector<glm::dvec3> contactList = incidentFaceInWorldSpace;
    for (auto & clippingPlane: sidePlanes) {

        std::vector<glm::dvec3> input = contactList;
        contactList.clear();

        for (unsigned int i = 0; i < input.size(); i++) {
            // get edge
            auto v1 = input[i];
            auto v2 = input[i + 1 == input.size() ? 0 : i + 1];

            // find intersection of edge v1v2 and the clippingPlane
            glm::dvec3 intersectionPoint;
            glm::dvec3 edgeDir = glm::dvec3(v2 - v1);
            if (glm::dot(clippingPlane.first, edgeDir) != 0) {
                
                double t = (glm::dot(clippingPlane.first, clippingPlane.second) - glm::dot(clippingPlane.first, v1)) / glm::dot(clippingPlane.first, glm::normalize(edgeDir));
                intersectionPoint = v1 + (glm::normalize(edgeDir) * t);
            }
                
            // do the clipping
            double distanceToV1 = SignedDistanceToPlane(clippingPlane.first, v1, clippingPlane.second);
            double distanceToV2 = SignedDistanceToPlane(clippingPlane.first, v2, clippingPlane.second);
            // std::cout << "d1 = " << distanceToV1  << ", d2 = " << distanceToV2 << " = distance from point " << glm::to_string(v1) << " to normal " << glm::to_string(clippingPlane.first) << " and plane point " << glm::to_string(clippingPlane.second) << "\n"; 

            if (distanceToV1 <= 0) { // then v1 is on the right side of the side plane and should stay
                contactList.push_back(v1);
                // std::cout << "\tkeeping.\n";
                if (distanceToV2 >= 0) { // then v2 is on the wrong side of the plane and thus the edge v goes through the side plane, we should add the intersection point
                    contactList.push_back(intersectionPoint);
                    // std::cout << "\tcase1: adding intersection point " << glm::to_string(intersectionPoint) << "\n";
                }
            }  
            else if (distanceToV2 <= 0) { // then v1 is on wrong side and v2 is on right side, so we should add point of intersection (v2 itself will be added on next iteration)
                contactList.push_back(intersectionPoint);
                // std::cout << "\tcase2: adding intersection point " << glm::to_string(intersectionPoint) << "\n";
            }
        
        } 

    }

    assert(contactList.size() > 0);
    // for (auto & v: contactList) {
        // std::cout << "After clipping, incident face has vertex " << glm::to_string(v) << "\n";
    // }

    // Lastly, verify for each contact point that it's actually inside the reference face and if it is, move contact points onto the reference face (idk why lel).
    std::vector<std::pair<glm::dvec3, double>> contactPoints; 
    for (auto & p: contactList) {
        
        // std::cout << "Testing point " << glm::to_string(p) << " against plane with normal " << glm::to_string(referenceNormal) << " and point " << glm::to_string(referenceFaceInWorldSpace.at(0)) << ".\n";
        
        assert(p.x != NAN);
        assert(p.y != NAN);
        assert(p.z != NAN);

        double distanceToPlane = glm::dot((glm::dvec3)referenceNormal, p - referenceFaceInWorldSpace.at(0));

        // std::cout << "Distance is " << distanceToPlane << ".\n";

        if (distanceToPlane <= 0) { // if contact point is inside the object
            contactPoints.push_back({p + ((glm::dvec3)referenceNormal * -distanceToPlane), -distanceToPlane});
        }
    }

    assert(contactPoints.size() > 0);
    return contactPoints;
}


// Calculates info about the collision, since EPA gives pretty mid contact points for face-face, face-edge, or edge-edge cases, causing weird rotational artifacts.
// When GJK determines a collision, we then use SAT to find the contact information.
// TODO: optimizations, concave support, non-vertex based support
// Contact points are in world space, will all be coplanar.
// returns a normal and a vector of pair<contactPosition, penetrationDepth> representing the contact surface.
// TODO: because sometimes GJK and SAT disagree about whether there's a collision because it's really close + FP precision errors, this function can say there wasn't actually a collision, which might not work.
std::optional<CollisionInfo> FindContact(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) 
{
    // if we have like spheres or something with no actual vertices that won't work for this, TODO fallback to EPA
    assert(collider1.physicsMesh->meshes.size() > 0);
    assert(collider2.physicsMesh->meshes.size() > 0);

    // Based on pg ~88 of https://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf

    // we already know they're colliding, we're just using SAT to find out how

    
    enum {
        Face1Collision = 0,
        Face2Collision = 1,
        EdgeCollision = 2
    } collisionType;

    // Face tests:
    // Test faces of collider1 against vertices of collider2
    // std::cout << "TESTING FACE 1\n";
    auto Face1Result = SatFaces(transform1, collider1, transform2, collider2); 
    assert(Face1Result.farthestDistance <= 0.1); // they must be colliding or we mad.    
    if (Face1Result.farthestDistance > 0) {std::cout << "sus amogus\n"; return std::nullopt;}

    // Test faces of collider2 against vertices of collider1
    // std::cout << "TESTING FACE 2\n";
    auto Face2Result = SatFaces(transform2, collider2, transform1, collider1); 
    assert(Face2Result.farthestDistance <= 0.1); // they must be colliding or we mad.
    if (Face2Result.farthestDistance > 0) {std::cout << "sus amogus\n"; return std::nullopt;}

    glm::vec3 farthestNormal(0, 0, 0); // in world space
    double farthestDistance;
    const std::vector<glm::vec3>* farthestFace = nullptr; // in model space

    if ((Face1Result.farthestDistance > Face2Result.farthestDistance)) {
        // std::cout << "Doing face1, distance " << Face1Result.farthestDistance << ".\n";
        farthestNormal = Face1Result.farthestNormal;
        farthestDistance = Face1Result.farthestDistance;
        farthestFace = Face1Result.farthestFace;
        collisionType = Face1Collision;
    }
    else {
        // std::cout << "Doing face2, distance " << Face2Result.farthestDistance << ".\n";
        farthestNormal = Face2Result.farthestNormal;
        farthestDistance = Face2Result.farthestDistance;
        farthestFace = Face2Result.farthestFace;
        collisionType = Face2Collision;
    }
    
    // just testing faces isn't enough for the case of edge-edge collision.
    // For each pair of <edge from collider1, edge from collider2>, we take the cross product of those to generate a plane/normal and test those planes too
    
    double farthestEdgeDistance = -FLT_MAX;
    glm::vec3 farthestEdgeNormal(0,0,0);
    glm::dvec3 farthestEdge1Origin(0, 0, 0), farthestEdge1Direction(0, 0, 0), farthestEdge2Origin(0, 0, 0), farthestEdge2Direction(0, 0, 0);

    for (auto & edge1: collider1.physicsMesh->meshes.at(0).edges) {
        // std::cout << " Edge1 is " << glm::to_string(edge1.first) << " to " << glm::to_string(edge1.second) << ".\n";
        for (auto & edge2: collider2.physicsMesh->meshes.at(0).edges) {
            // put edges in world space
            glm::dvec3 edge1OriginWorld = transform1.GetPhysicsModelMatrix() * glm::dvec4(edge1.first, 1.0);
            glm::dvec3 edge1bWorld = transform1.GetPhysicsModelMatrix() * glm::dvec4(edge1.second, 1.0);
            glm::dvec3 edge1DirectionWorld = glm::normalize(edge1bWorld - edge1OriginWorld);
            glm::dvec3 edge2OriginWorld = transform2.GetPhysicsModelMatrix() * glm::dvec4(edge2.first, 1.0);
            glm::dvec3 edge2bWorld = transform2.GetPhysicsModelMatrix() * glm::dvec4(edge2.second, 1.0);
            glm::dvec3 edge2DirectionWorld = glm::normalize(edge2bWorld - edge2OriginWorld);

            glm::vec3 normalInWorldSpace =  glm::cross(edge1DirectionWorld, edge2DirectionWorld);
            if (glm::length(normalInWorldSpace) < 0.0001) { // if the edges are parallel, cross product is zero and we can't get a normal
                // std::cout << "Skipping parallel edge pair.\n";
                continue;
            }
            normalInWorldSpace = glm::normalize(normalInWorldSpace);
            // make sure all normals go out of collider1, so the sign of our distance calculations is consistent
            // TODO:
                // transform1 and 2 are both wrong
            if (glm::dot(normalInWorldSpace, glm::vec3(edge1OriginWorld - transform1.Position())) < 0.0) { // if dot product between center of model to vertex and the normal is < 0, normal is opposite direction of model to vertex and needs to be flipped
                // std::cout << "\tFlipped normal.\n";
                normalInWorldSpace *= -1;
            }
            else {
                // std::cout << "\tDid not flip, dot product was " << glm::dot(normalInWorldSpace, glm::vec3(edge1OriginWorld - transform1.Position())) << ".\n";
            }

            // often times the edge pair will create a normal that is the same as a face normal, in which case we can skip it.
            // if (glm::length(glm::cross(normalInWorldSpace, farthestNormal)) == 0) {
            //     // std::cout << "\tSkipping redundant edge pair.\n";
            //     continue;
            // }
            
            glm::dvec3 collider2Vertex = FindFarthestVertexOnObject(-normalInWorldSpace, transform2, collider2);

            // using edge1OriginWorld doesn't work here because it's not actually a point on the plane we want to test, so instead we using support function?
             glm::dvec3 collider1Vertex = FindFarthestVertexOnObject(normalInWorldSpace, transform1, collider1);
            double distance = SignedDistanceToPlane(normalInWorldSpace, collider2Vertex, collider1Vertex);
            // distance has to be negative if they colliding so use opposite normal if its not
            // if (distance > 0) {
            //     distance *= -1;
            //     normalInWorldSpace *= -1;
            // }
            
            if (distance > farthestEdgeDistance) {
                // std::cout << "\tEDGY: Normal " << glm::to_string(normalInWorldSpace) << " replaced normal " << glm::to_string(farthestEdgeNormal) << "with distance " << distance << " being bigger than " << farthestEdgeDistance << "\n";
                // std::cout << "\t\tUsed vertex " << glm::to_string(collider2Vertex) << " and point on plane " << glm::to_string(edge1OriginWorld) << ".\n";
                // std::cout << "\t\tOrigins are " << glm::to_string(edge1OriginWorld) << " and " << glm::to_string(edge2OriginWorld) << ".\n";
                // std::cout << "\t\tDirections are " << glm::to_string(edge1DirectionWorld) << " and " << glm::to_string(edge2DirectionWorld) << ".\n";
                farthestEdgeDistance = distance;
                // if (distance > 0) {
                //     // std::cout << "NO EDGE COLLISION\n";
                //     goto NoEdgeCollision;
                // }
                farthestEdgeNormal = normalInWorldSpace;
                farthestEdge1Origin = collider1Vertex;
                farthestEdge1Direction = edge1DirectionWorld;
                farthestEdge2Origin = collider2Vertex;
                farthestEdge2Direction = edge2DirectionWorld;
            }

            assert(distance <= 0.1); // if distance > 0, we have no collision and this function won't work
            if (Face1Result.farthestDistance > 0) {std::cout << "sus amogus\n"; return std::nullopt;}
        }
    }

    

    const double FACE_COLLISION_BIAS = 0.001; // face collisions preferable to edge-edge ones
    if (farthestEdgeDistance > farthestDistance + FACE_COLLISION_BIAS) { 
        collisionType = EdgeCollision;
        farthestDistance = farthestEdgeDistance;
        farthestNormal = farthestEdgeNormal;
    }

    // std::cout << "Farthest edge distance is " << farthestEdgeDistance << ".\n";

    assert(farthestDistance <= 0.1); // if this was > 0, then they wouldn't be colliding, and if you called this function they better be colliding

    std::vector<std::pair<glm::dvec3, double>> contactPoints;

    // figure whether a face of collider1 hit collider2, a face of collider2 hit collider1, or if their edges hit each other, and then calculate contact points.
    // smallest distance is the right one.
    switch (collisionType) {
        case Face1Collision:
        // std::cout << "FACE1 COLLISION\n"; 
        return CollisionInfo {.collisionNormal = farthestNormal, .contactPoints = ClipFaceContactPoints(farthestNormal, farthestFace, transform1, transform2, collider2)};
        break;
        case Face2Collision:
        // std::cout << "FACE2 COLLISION\n";
        return CollisionInfo {.collisionNormal = -farthestNormal, .contactPoints = ClipFaceContactPoints(farthestNormal, farthestFace, transform2, transform1, collider1)};
        break;
        {case EdgeCollision:
        // std::cout << "EDGE COLLISION\n";
        // std::printf("Edge is %f vs face %f", farthestEdgeDistance, farthestDistance);
        // There are two edges colliding in this case. Contact point is average of closest point on edge1 to edge2 and closest point on edge2 to edge1.
        // see https://en.wikipedia.org/wiki/Skew_lines#Nearest_points for formula to get those closest points
        // std::cout << "Edge info: #1 at " << glm::to_string(farthestEdge1Origin) << " towards " << glm::to_string(farthestEdge1Direction) << ".\n";
        // std::cout << "Edge info: #2 at " << glm::to_string(farthestEdge2Origin) << " towards " << glm::to_string(farthestEdge2Direction) << ".\n";
        auto n2 = glm::cross(farthestEdge2Direction, (glm::dvec3)farthestNormal);
        
 
        auto closestPointOnEdge1ToEdge2 = farthestEdge1Origin + farthestEdge1Direction * (glm::dot((farthestEdge2Origin - farthestEdge1Origin), n2)/glm::dot(n2, farthestEdge1Direction));
        auto n1 = glm::cross(farthestEdge1Direction, (glm::dvec3)farthestNormal);
        

        auto closestPointOnEdge2ToEdge1 = farthestEdge2Origin + farthestEdge2Direction * (glm::dot((farthestEdge1Origin - farthestEdge2Origin), n1)/glm::dot(n1, farthestEdge2Direction));
        

        auto point = (closestPointOnEdge1ToEdge2 + closestPointOnEdge2ToEdge1) * 0.5;
        // std::cout << "EDGE: n2= " << glm::to_string(n2) << ".\n";
        // std::cout << "EDGE: calculated point " << glm::to_string(closestPointOnEdge1ToEdge2) << ".\n";

        assert(!std::isnan(closestPointOnEdge1ToEdge2.x));
        assert(!std::isnan(closestPointOnEdge1ToEdge2.y));
        assert(!std::isnan(closestPointOnEdge1ToEdge2.z));   
        assert(!std::isnan(closestPointOnEdge2ToEdge1.x));
        assert(!std::isnan(closestPointOnEdge2ToEdge1.y));
        assert(!std::isnan(closestPointOnEdge2ToEdge1.z));  

        return CollisionInfo {.collisionNormal = farthestNormal, .contactPoints = {{point, abs(farthestEdgeDistance)}}};
        break;}
        default:
        // shut up compiler warning for reaching end of non-void function
        std::cout << "GJK at line " << __LINE__ << " has messed up hard.\n";
        abort();
    }


}   

//  EPA algorithm, used to get collision normals/penetration depth, explained here: https://winter.dev/articles/epa-algorithm
// TODO: concave support
// CollisionInfo EPA(
//     std::vector<std::array<glm::dvec3, 3>>& simplex, // first dvec3 in each array is actual simplex point on the minkoskwi difference, the other 2 are the collider points whose difference is that point, we need those for contact points
//     const TransformComponent& transform1,
//     const SpatialAccelerationStructure::ColliderComponent& collider1,
//     const TransformComponent& transform2,
//     const SpatialAccelerationStructure::ColliderComponent& collider2
// ) 
// {
//     // Simplex is no longer a simplex and is just a convex polytope (3d polygon) made from (more than 4) points on the Minkoski difference.
//     auto & polytope = simplex;

//     // To find the normal, we must progressively expand the simplex, which neccesitates knowing the faces of the simplex so that we can calculate proper normals
//     std::vector<unsigned int> faces = {
// 		0, 1, 2,
// 		0, 3, 1,
// 		0, 2, 3,
// 		1, 3, 2
// 	};

//     // vector<pair of normal + distance>, minFace = index to face with min distance
// 	auto [normals, minFace] = GetFaceNormals(polytope, faces);
//     assert(normals.size() == 4); // simplex should have 4 vertices and 4 faces

//     glm::dvec3 minNormal;
// 	double minDistance = FLT_MAX;
//     unsigned short nIterations = 0;
// 	while (minDistance == FLT_MAX) {
//         // assert(nIterations < 64);
//         // std::cout << "iteration " << nIterations << "\n";
//         nIterations+=1;

// 		minNormal   = normals.at(minFace).first;
// 		minDistance = normals.at(minFace).second;

//         if (nIterations > 64) {
//             std::cout << "EPA failed\n";
//             break;
//         }
 
// 		auto support = NewSimplexPoint(minNormal, transform1, collider1, transform2, collider2);
// 		double sDistance = glm::dot(minNormal, support[0]);
 
//         // std::cout << "Polytope: ";
//         for (auto & p: polytope) {
//             std::cout << glm::to_string(p[0]) << ", ";
//         }
//         // std::cout << "\n";
//         // std::cout << "Min normal is " << glm::to_string(minNormal) << "\n";
        
//         // std::cout << "S distance " << sDistance << " created by dot of " << glm::to_string(minNormal) << " and support " << glm::to_string(support[0]) << "\n";

//         // std::cout << "Distance is " << sDistance << " - " << minDistance << ".\n";
// 		if (abs( sDistance - minDistance) > 0.001f) {
// 			minDistance = FLT_MAX;
//             std::vector<std::pair<unsigned int, unsigned int>> uniqueEdges;
//             // std::cout << "before loop size is " << uniqueEdges.size() << "\n";

//             assert(normals.size() > 0);
// 			for (unsigned int i = 0; i < normals.size(); i++) {

//                  // check if after adding that support point, this face is no longer in the polytope and needs to go.
// 				// if (glm::dot(normals[i].first, support[0]) > glm::dot(normals[i].first, polytope[faces[i * 3]][0])) {
//                 if (glm::dot(normals[i].first, support[0]) > 0) {
// 					unsigned int f = i * 3;

//                     // For all of the edges of this face, 
// 					AddIfUniqueEdge(uniqueEdges, faces, f,     f + 1);
// 					AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
// 					AddIfUniqueEdge(uniqueEdges, faces, f + 2, f    );
//                     // std::cout << "after those adds there are " << uniqueEdges.size() << "\n";

// 					faces[f + 2] = faces.back(); faces.pop_back();
// 					faces[f + 1] = faces.back(); faces.pop_back();
// 					faces[f    ] = faces.back(); faces.pop_back();

//                     // std::cout << "uh oh bois we deleting a normal.\n";
// 					normals[i] = normals.back(); // pop-erase
// 					normals.pop_back();

// 					i--;
// 				}
//                 // std::cout << "that just happened, size " << uniqueEdges.size() << "\n";
// 			}
//             // std::cout << "after everything there are still " << uniqueEdges.size() << "\n";
//             assert(uniqueEdges.size() > 0);
//             std::vector<unsigned int> newFaces;
// 			for (auto [edgeIndex1, edgeIndex2] : uniqueEdges) {
// 				newFaces.push_back(edgeIndex1);
// 				newFaces.push_back(edgeIndex2);
// 				newFaces.push_back(polytope.size());
// 			}
            
			 
// 			polytope.push_back(support);
            
//             assert(newFaces.size() > 0);
//             auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

//             double oldMinDistance = FLT_MAX;
//             for (unsigned int i = 0; i < normals.size(); i++) {
//                 if (normals[i].second < oldMinDistance) {
//                     oldMinDistance = normals[i].second;
//                     minFace = i;
//                 }
//             }

//             if (newNormals.at(newMinFace).second < oldMinDistance) {
//                 minFace = newMinFace + normals.size();
//             }

//             // std::cout << "Extending normals.\n";
//             // std::cout << "Was size " << normals.size() << ".\n";
//             faces  .insert(faces  .end(), newFaces  .begin(), newFaces  .end());
//             normals.insert(normals.end(), newNormals.begin(), newNormals.end());
//             // std::cout << "Now its size " << normals.size() << ".\n";
			
            
// 		}
// 	}

//     assert((minNormal != glm::dvec3(0,0,0)));
//     // assert(minDistance != 0); // TODO: PROBABLY REMOVE

//     // find collision point
//     // get verts of closest triangle to origin
//     auto& a = polytope[faces[minFace * 3]];
//     auto& b = polytope[faces[minFace * 3 + 1]];
//     auto& c = polytope[faces[minFace * 3 + 2]];

//     // DebugPlacePointOnPosition({a[0]}, {1.0, 0.4, 1.0, 1.0});
//     // DebugPlacePointOnPosition({b[0]}, {1.0, 0.7, 1.0, 1.0});
//     // DebugPlacePointOnPosition({c[0]}, {1.0, 1.0, 1.0, 1.0});

//     // DebugPlacePointOnPosition({a[1]}, {0.2, 1.0, 0.2, 1.0});
//     // DebugPlacePointOnPosition({b[1]}, {0.2, 1.0, 0.2, 1.0});
//     // DebugPlacePointOnPosition({c[1]}, {0.2, 1.0, 0.2, 1.0});

//     // DebugPlacePointOnPosition({a[2]}, {1.0, 0.4, 0.0, 1.0});
//     // DebugPlacePointOnPosition({b[2]}, {1.0, 0.7, 0.0, 1.0});
//     // DebugPlacePointOnPosition({c[2]}, {1.0, 1.0, 0.0, 1.0});

//     // std::printf("Min face vertices are %f %f %f and %f %f %f, and %f %f %f \n", a[0].x, a[0].y, a[0].z, b[0].x, b[0].y, b[0].z, c[0].x, c[0].y, c[0].z);
//     // std::printf("Min face vertices for support 1 are %f %f %f and %f %f %f, and %f %f %f \n", a[1].x, a[1].y, a[1].z, b[1].x, b[1].y, b[1].z, c[1].x, c[1].y, c[1].z);
//     // std::printf("Min face vertices for support 2 are %f %f %f and %f %f %f, and %f %f %f \n", a[2].x, a[2].y, a[2].z, b[2].x, b[2].y, b[2].z, c[2].x, c[2].y, c[2].z);
//     // std::cout << "\tHmm, minNormal is " << glm::to_string(minNormal) << " but the minFace's normal we got is " << glm::to_string(glm::normalize(glm::cross(b[0] - a[0], c[0] - a[0]))) << ".\n";

//     // Project origin onto plane abc to get point of contact in minkoski space
//     auto planeToOrigin = -a[0];
//     auto distance = glm::dot(planeToOrigin, minNormal);
//     auto projectedPoint = -minNormal * distance;
//     // auto projectedPoint = glm::normalize(-minNormal * glm::dot(-a[0], minNormal));
//     // std::cout << "Projected point " << glm::to_string(projectedPoint) << "\n";
    

//     // put that point of contact in barycentric coordinates (meaning its a mix of the triangle vertices), so that we can get out of minkoski space and into world space
//     auto ab = b[0] - a[0];
//     auto ac = c[0] - a[0];
//     auto ao = projectedPoint - a[0];

//     glm::dvec3 pointForObj2;
//     glm::dvec3 pointForObj1;

//     // in the case of edge-face collision, we can't use barycentric coordinates to find the contact point because we have a degenerate triangle (the edge is a degenerate triangle) 
    
    
//     // if (glm::length2(glm::cross(b[2] - a[2], c[2] - a[2])) == 0) {
//     //     // TODO: this assumes that a and c are always the identical points, which is literally wrong
//     //     //assert(a[2] != b[2]);
//     //     // std::cout << "Executing EPA degenerate contact point case.\n";
//     //     // std::cout << "\tCross is  = " << glm::to_string(glm::cross(b[2] - a[2], b[2] - projectedPoint)) << "\n";
//     //     // // std::cout << "\tLerp between " << glm::to_string(a[2]) << " and " << glm::to_string(b[2]) << "\n";
//     //     // std::cout << "\tLerp between In minkoski space " << glm::to_string(a[0]) << " and " << glm::to_string(b[0]) << "\n";
//     //     // std::cout << "\tAB = " << glm::to_string(ab) << "\n";
//     //     // std::cout << "\tAB for actual world space = " << glm::to_string(b[2] - a[2]) << "\n";
//     //     // std::cout << "\tP = " << glm::to_string(projectedPoint) << "\n";
//     //     // auto lerpAmount = glm::length(projectedPoint - a[0])/glm::length(ab);
//     //     // std::cout << "\tlerp amount = " << /*glm::to_string*/(lerpAmount) << "\n";
//     //     pointForObj2 = a[2] + 0.5 * (b[2] - a[2]);
//     // }
//     // else {
//         // std::cout << "AB = " << glm::to_string(ab) << ", AC = " << glm::to_string(ac) << ", AO = " << glm::to_string(ao) << ".\n";
    
//         double d00 = glm::dot(ab, ab);
//         double d01 = glm::dot(ab, ac);
//         double d11 = glm::dot(ac, ac);
//         double d20 = glm::dot(ao, ab);
//         double d21 = glm::dot(ao, ac);
//         double denom = d00 * d11 - d01 * d01;

//         // uvw is barycentric coords aka a mixture of the triangle vertices that averages out to the point we got
//         double v = (d11 * d20 - d01 * d21) / denom;
//         double w = (d00 * d21 - d01 * d20) / denom;
//         double u = 1.0f - v - w;
//         // we use that mixture with the triangle vertices that AREN'T in minkoski space to get the real contact point
//         // std::cout << "We got " << glm::to_string((a[0] * u) + (b[0] * v) + (c[0] * w)) << " vs " << glm::to_string(projectedPoint);
//         pointForObj1 = (a[1] * u) + (b[1] * v) + (c[1] * w);  
//         pointForObj2 = (a[2] * u) + (b[2] * v) + (c[2] * w); 
//         std::cout << "Its either " << glm::to_string(pointForObj1) << " or " << glm::to_string(pointForObj2) << ".\n";
//         std::printf("Barycentric coords are %f %f %f \n", v, w, u);
//         // std::printf("Considering support points %f %f %f and %f %f %f, and %f %f %f \n", a[2].x, a[2].y, a[2].z, b[2].x, b[2].y, b[2].z, c[2].x, c[2].y, c[2].z);

//         //auto point = (a[2] * gamma) + (b[2] * beta) + (c[2] * alpha);
//     // }

//     // return CollisionInfo({
//     //     .collisionNormal = minNormal,
//     //     .contactPoints = {pointForObj2},
//     //     .penetrationDepth = minDistance // TODO: add 0.0001f?
//     // });
// }
 
// this article actually does a really good job of explaining the GJK algorithm.
// https://cse442-17f.github.io/Gilbert-Johnson-Keerthi-Distance-Algorithm/
// TODO: concave support
std::optional<CollisionInfo> IsColliding(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
) 
{
    // std::cout << "HI: testing collision between #1 = " << glm::to_string(transform1.Position()) << " and #2 = " << glm::to_string(transform2.Position()) << "\n";
    // first dvec3 in each array is actual simplex point on the minkoskwi difference, the other 2 are the collider points whose difference is that point, we need those for contact points
    std::vector<std::array<glm::dvec3, 3>> simplex;

    // Search direction is in WORLD space.
    glm::dvec3 searchDirection = glm::normalize(glm::dvec3 {1, 1, 1}); // arbitrary starting direction
    // std::printf("\tInitial search direction %f %f %f\n", searchDirection.x, searchDirection.y, searchDirection.z);

    // add starting point to simplex
        // Subtracting findFarthestVertex(direction) from findFarthestVertex(-direction) gives a point on the Minoski difference of the two objects.
        // If the minowski difference of the 2 objects contains the origin, there is a point where the two positions subtracted from each other = 0, meaning the two objects are colliding.
        // Again, check the link above if you don't get it.
        // The simplex is just (in 3d) 4 points in the minoski difference that will be enough to determine whether the objects are colliding.
    simplex.push_back(NewSimplexPoint(searchDirection, transform1, collider1, transform2, collider2));
    // std::cout << "INIT: Searched in " << glm::to_string(searchDirection) << " to get point " << glm::to_string(simplex.back()[0]) << "\n";
    // make new search direction go from simplex towards origin
    searchDirection = glm::normalize(-simplex.back()[0]);

    while (true) {

        // std::cout << "\tSimplex: ";
        // for (auto & p: simplex) {
            // std::cout << glm::to_string(p[0]) << " from " << glm::to_string(p[1]) << " - " << glm::to_string(p[2]) << ", ";
        // }
        // std::cout << "\n";

        // get new point for simplex
        auto newSimplexPoint = NewSimplexPoint(searchDirection, transform1, collider1, transform2, collider2);
        // std::printf("Going in direction %f %f %f\n", searchDirection.x, searchDirection.y, searchDirection.z);
        //  std::cout << "\tSearched in " << glm::to_string(searchDirection) << " to get point " << glm::to_string(newSimplexPoint[0]) << " from " << glm::to_string(newSimplexPoint[1]) << " - " << glm::to_string(newSimplexPoint[2]) << "\n";

        // this is the farthest point in this direction, so if it didn't get past the origin, then origin is gonna be outside the minoski difference meaning no collision.
        if (glm::dot(newSimplexPoint[0], searchDirection) <= 0) {
            // std::cout << "GJK failed with " << simplex.size() << " vertices.\n\n";
            // while (true) {}kk
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
            // std::cout << "Executing line case.\n";
            LineCase(simplex, searchDirection); 
            break;
            case 3:
            // std::cout << "Executing triangle case.\n";
            TriangleCase(simplex, searchDirection);
            break;
            case 4:
            // std::cout << "Executing tetrahedron case.\n";
            if (TetrahedronCase(simplex, searchDirection)) { // this function is not void like the others, returns true if collision confirmed
                // // to make SAT stop crashing because distance is barely > 0 somehow:
                // std::array<int, 12> faces {0, 1, 2,   0, 1, 3,   0, 2, 3,   1, 2, 3};
                // for (unsigned int f = 0; f < 4; f++) {
                //     auto p1 = simplex[faces[f * 3]][0];
                //     auto p2 = simplex[faces[f * 3 + 1]][0];
                //     auto p3 = simplex[faces[f * 3 + 2]][0];

                //     auto normal = glm::normalize(glm::cross(p1 - p2, p2 - p3));
                //     auto distance = SignedDistanceToPlane(normal, {0, 0, 0}, p1);
                //     // std::cout << "bru distance is " << distance << " from normal " << glm::to_string(normal) << "\n";
                //     if (std::abs(distance) < 1.0e-06) {
                //         return std::nullopt;
                //     }
                // }

                // return EPA(simplex, transform1, collider1, transform2, collider2);
                // std::cout << "THERE IS A COLLISION\n";
                // std::cout << "Positions are #1 = " << glm::to_string(transform1.Position()) << " and #2 = " << glm::to_string(transform2.Position()) << "\n";
                auto result = FindContact(transform1, collider1, transform2, collider2);
                if (!result) {
                    std::cout << "SAT and GJK disagreed, uh oh.\n";
                }
                return result;
            }
            break;
            default:
            std::cout << "GJK: WHAT\n";
            abort();
            break;
        }
    }
}