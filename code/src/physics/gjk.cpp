#include "gjk.hpp"
#include <algorithm>
#include <array>
#include "debug/assert.hpp"
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <utility>
#include <vector>
#include <iostream>
#include "gameobjects/gameobject.hpp"
#include "debug/log.hpp"
#include <unordered_map>
#include <mutex>
// DEAR GOD. YOU DON'T KNOW. A MONTH AND A HALF WAS SPENT SUFFERING IN THIS FILE. I NEVER WANT TO TOUCH THIS AGAIN.
// DECEMBER TO FEBRUARY 2ND. FINALLY.
// feb 14 it wasn't enough apparently
// november 29 of 2025 i'm (not) laughing and wondering which year these comments were written in. i won't use git blame. 

// the GJK support function. returns farthest vertex (in world space) along a directional vector 
glm::dvec3 FindFarthestVertexOnObject(const glm::dvec3& directionInWorldSpace, const glm::mat3x3& inverseNormMatrix, const glm::dmat4x4& worldMatrix, const ColliderComponent& collider) {
    // the physics mesh's vertices are obviously in model space, so we must put search direction in model space.
    // this way, to find the farthest vertex on a 10000-vertex mesh we don't need to do 10000 vertex transformations
    // TODO: this is still O(n vertices) complexity

    Assert(!std::isnan(directionInWorldSpace.x)); 
    Assert(!std::isnan(directionInWorldSpace.y)); 
    Assert(!std::isnan(directionInWorldSpace.z)); 

    // std::cout << "Normal matrix is " << glm::to_string(transform.GetNormalMatrix()) << "\n";
    glm::vec3 directionInModelSpace = inverseNormMatrix * directionInWorldSpace;

    Assert(!std::isnan(directionInModelSpace.x)); 
    Assert(!std::isnan(directionInModelSpace.y)); 
    Assert(!std::isnan(directionInModelSpace.z)); 

    //TODO: concave support
    glm::vec3 farthestVertex = collider.physicsMesh->meshes.at(0)->FindFarthestPointOnObject(directionInModelSpace);

    

    //// std::cout << "Model matrix is " << glm::to_string(transform.GetPhysicsModelMatrix()) << "\n";
    // std::cout << "Support: farthest vertex in direction " << glm::to_string(directionInModelSpace) << " is " << glm::to_string(farthestVertex) << "\n";

    // put returned point in world space
    auto farthestVertexInWorldSpace = glm::dvec3(worldMatrix * glm::dvec4(farthestVertex.x, farthestVertex.y, farthestVertex.z, 1));
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
    const ColliderComponent& collider1,
    const ColliderComponent& collider2,
    const glm::mat3x3& invNorm1,
    const glm::mat3x3& invNorm2,
    const glm::dmat4x4& world1,
    const glm::dmat4x4& world2
) {
    // DebugLogInfo("Generating new simplex point.");
    auto a = FindFarthestVertexOnObject(searchDirection, invNorm1, world1, collider1);
    auto b = FindFarthestVertexOnObject(-searchDirection, invNorm2, world2, collider2);

    Assert(!std::isnan(a.x)); 
    Assert(!std::isnan(a.x)); 
    Assert(!std::isnan(a.x)); 



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
		edges.erase(reverse);
        return;
	}

    /*auto forward = std::find(
        edges.begin(),
        edges.end(),
        std::make_pair(faces[a], faces[b])
    );
    if (forward != edges.end()) {
        edges.erase(forward);
        return;
    }*/
	
    edges.emplace_back(faces[a], faces[b]);
}

// Used by EPA to get (take a guess) face normals.
// Returns vector of pair {normal, distance to face} and index of the closest normal.
std::pair<std::vector<std::pair<glm::dvec3, double>>, unsigned int> GetFaceNormals(
    const std::vector<std::array<glm::dvec3, 3>>& polytope, 
    const std::vector<unsigned int>& faces) 
    {
	std::vector<std::pair<glm::dvec3, double>> normals;
    Assert(faces.size() > 0);
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

    Assert(normals.size() > 0);
	return { normals, minTriangle };
}

// used by FindContact()
double SignedDistanceToPlane(glm::dvec3 planeNormal, glm::dvec3 point, glm::dvec3 pointOnPlane) {
    return glm::dot(planeNormal, point - pointOnPlane);
}



//  EPA algorithm, used to get collision normals/penetration depth, explained here: https://winter.dev/articles/epa-algorithm
// TODO: concave support
CollisionInfo EPA(
    std::vector<std::array<glm::dvec3, 3>>& simplex, // first dvec3 in each array is actual simplex point on the minkoskwi difference, the other 2 are the collider points whose difference is that point, we need those for contact points
    const ColliderComponent& collider1,
    const ColliderComponent& collider2,
    const glm::mat3x3& invNorm1,
    const glm::mat3x3& invNorm2,
    const glm::dmat4x4& world1,
    const glm::dmat4x4& world2,
    const glm::dmat4x4& invWorld2,
    const glm::mat3x3& norm1
) 
{
    Assert(simplex.size() == 4);

    // Simplex is no longer a simplex and is just a convex polytope (3d polygon) made from (more than 4) points on the Minkoski difference.
    auto & polytope = simplex;

    

    // To find the normal, we must progressively expand the simplex, which neccesitates knowing the faces of the simplex so that we can calculate proper normals
    std::vector<unsigned int> faces = {
 	0, 1, 2,
 	0, 3, 1,
 	0, 2, 3,
 	1, 3, 2
    };

    //for (unsigned fI = 0; fI < faces.size(); fI+=3 ) {
        //auto g = DebugPlaceTriangle({polytope[faces[fI]][0], polytope[faces[fI+1]][0] , polytope[faces[fI+2]][0] }, {1, 1, 0, 1});
        //g->RawGet<TransformComponent>()->SetPos(glm::dvec3(5, 1, 5));
    //}

        // vector<pair of normal + distance>, minFace = index to face with min distance
    auto [normals, minFace] = GetFaceNormals(polytope, faces);
    Assert(normals.size() == 4); // simplex should have 4 vertices and 4 faces

    glm::dvec3 minNormal;
    double minDistance = FLT_MAX;
    unsigned short nIterations = 0;
    while (minDistance == FLT_MAX) {
        // Assert(nIterations < 64);
        // std::cout << "iteration " << nIterations << "\n";
        nIterations+=1;

 	    minNormal   = normals.at(minFace).first;
 	    minDistance = normals.at(minFace).second;

        if (nIterations > 64) {
            //DebugLogError("EPA failed (iterations exceeded)");
            break;
        }

        //for (unsigned fI = 0; fI < faces.size(); fI += 3) {
            //auto g = DebugPlaceTriangle({ polytope[faces[fI]][0], polytope[faces[fI+1]][0] , polytope[faces[fI+2]][0] }, { 1, 1, 0, 1 });
            //g->RawGet<TransformComponent>()->SetPos(glm::dvec3(nIterations * 5, 1, 1));
        //}
        //DebugPlacePointOnPosition(glm::dvec3(nIterations * 5, 1, 1), {1, 0, 0, 1});
 
        //DebugPlaceLine({0, 0, 0}, minNormal, {1, 0, 0, 1})->RawGet<TransformComponent>()->SetPos(glm::dvec3(nIterations * 5, 1, 1));
 	    auto support = NewSimplexPoint(minNormal, collider1, collider2, invNorm1, invNorm2, world1, world2);   
        //DebugPlacePointOnPosition(support[0] + glm::dvec3(nIterations * 5, 1, 1), {0, 0, 1, 1});
 	    double sDistance = glm::dot(minNormal, support[0]);
    
 	    if (abs( sDistance - minDistance) > 0.00001) {
            std::vector<std::pair<unsigned int, unsigned int>> uniqueEdges;

            Assert(normals.size() > 0);
 		    for (unsigned int i = 0; i < normals.size(); i++) {

                // check if after adding that support point, this face is no longer in the polytope and needs to go.
 			    // if (glm::dot(normals[i].first, support[0]) > glm::dot(normals[i].first, polytope[faces[i * 3]][0])) {
                
                //auto mid = polytope[faces[i * 3]][0] + polytope[faces[i * 3+1]][0] + polytope[faces[i * 3+2]][0];
                //mid /= 3.0;
                //DebugPlaceLine(mid, mid + normals[i].first * 0.3, {1, 0, 1, 1})->RawGet<TransformComponent>()->SetPos(glm::dvec3(nIterations * 5, 1, 1));
                //auto norm = glm::cross(polytope[faces[i*3 + 2]][0] - polytope[faces[i * 3]][0], polytope[faces[i * 3 + 1]][0] - polytope[faces[i * 3]][0])
                
                //TestBillboardUi(mid + glm::dvec3(nIterations * 5, 1, 1), std::to_string(glm::dot(normals[i].first, glm::normalize(support[0]))));
                
                if (SignedDistanceToPlane(normals[i].first, polytope[faces[i*3]][0], support[0]) < 0) {
 				    unsigned int f = i * 3;

                    // For all of the edges of this face, 
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
            if (uniqueEdges.size() == 0) {
                DebugLogError("EPA failed (edges)");
                break;
            }
            //Assert(uniqueEdges.size() > 0);
            std::vector<unsigned int> newFaces;
 		    for (auto [edgeIndex1, edgeIndex2] : uniqueEdges) {
 			    newFaces.push_back(edgeIndex1);
 			    newFaces.push_back(edgeIndex2);
 			    newFaces.push_back(polytope.size());
                //DebugPlaceLine( polytope[edgeIndex1][0], polytope[edgeIndex2][0], {1, 0, 1, 1})->RawGet<TransformComponent>()->SetPos(glm::dvec3(nIterations * 5, 1.01, 1));
 		    
                //DebugPlaceLine(polytope[edgeIndex1][0], support[0], {0, 0, 1, 1})->RawGet<TransformComponent>()->SetPos(glm::dvec3(nIterations * 5, 1, 1));
                //DebugPlaceLine(polytope[edgeIndex2][0], support[0], { 0, 0, 1, 1 })->RawGet<TransformComponent>()->SetPos(glm::dvec3(nIterations * 5, 1, 1));

            }
            
 		    polytope.push_back(support);
            
            Assert(newFaces.size() > 0);
            auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

            double oldMinDistance = FLT_MAX;
            for (unsigned int i = 0; i < normals.size(); i++) {
                if (normals[i].second < oldMinDistance) {
                    oldMinDistance = normals[i].second;
                    minFace = i;
                }
            }

            if (newNormals.at(newMinFace).second < oldMinDistance) {
                minFace = newMinFace + normals.size();
            }

            // std::cout << "Extending normals.\n";
            // std::cout << "Was size " << normals.size() << ".\n";
            faces  .insert(faces  .end(), newFaces  .begin(), newFaces  .end());
            normals.insert(normals.end(), newNormals.begin(), newNormals.end());
            // std::cout << "Now its size " << normals.size() << ".\n";
			
            minDistance = FLT_MAX;
 	    }
    }

    Assert((minNormal != glm::dvec3(0,0,0)));
    // Assert(minDistance != 0); // TODO: PROBABLY REMOVE

    if (faces.size() == 0) {
        DebugLogError("EPA failed (faces)");
        Assert(false);
    }

    // find collision point
    // get verts of closest triangle to origin
    auto& a = polytope[faces[minFace * 3]];
    auto& b = polytope[faces[minFace * 3 + 1]];
    auto& c = polytope[faces[minFace * 3 + 2]];

    // Project origin onto plane abc to get point of contact in minkoski space
    auto planeToOrigin = -a[0];
    auto distance = glm::dot(planeToOrigin, minNormal);
    auto projectedPoint = -minNormal * distance;
    
    // put that point of contact in barycentric coordinates (meaning its a mix of the triangle vertices), so that we can get out of minkoski space and into world space
    auto ab = b[0] - a[0];
    auto ac = c[0] - a[0];
    auto ao = projectedPoint - a[0];

    // in the case of edge-face collision, we can't use barycentric coordinates to find the contact point because we have a degenerate triangle (the edge is a degenerate triangle) 
    glm::dvec3 pointForObj1;
    glm::dvec3 pointForObj2;
    //Assert(glm::length2(glm::cross(b[2] - a[2], c[2] - a[2])) > 0.0001);
    //Assert(glm::length2(glm::cross(c[2] - b[2], c[2] - a[2])) > 0.0001);

    // if (glm::length2(glm::cross(b[2] - a[2], c[2] - a[2])) == 0) {
    //     // TODO: this assumes that a and c are always the identical points, which is literally wrong
    //     //Assert(a[2] != b[2]);
    //     // std::cout << "Executing EPA degenerate contact point case.\n";
    //     // std::cout << "\tCross is  = " << glm::to_string(glm::cross(b[2] - a[2], b[2] - projectedPoint)) << "\n";
    //     // // std::cout << "\tLerp between " << glm::to_string(a[2]) << " and " << glm::to_string(b[2]) << "\n";
    //     // std::cout << "\tLerp between In minkoski space " << glm::to_string(a[0]) << " and " << glm::to_string(b[0]) << "\n";
    //     // std::cout << "\tAB = " << glm::to_string(ab) << "\n";
    //     // std::cout << "\tAB for actual world space = " << glm::to_string(b[2] - a[2]) << "\n";
    //     // std::cout << "\tP = " << glm::to_string(projectedPoint) << "\n";
    //     // auto lerpAmount = glm::length(projectedPoint - a[0])/glm::length(ab);
    //     // std::cout << "\tlerp amount = " << /*glm::to_string*/(lerpAmount) << "\n";
    //     pointForObj2 = a[2] + 0.5 * (b[2] - a[2]);
    // }
    // else {
        // std::cout << "AB = " << glm::to_string(ab) << ", AC = " << glm::to_string(ac) << ", AO = " << glm::to_string(ao) << ".\n";
    
    double d00 = glm::dot(ab, ab);
    double d01 = glm::dot(ab, ac);
    double d11 = glm::dot(ac, ac);
    double d20 = glm::dot(ao, ab);
    double d21 = glm::dot(ao, ac);
    double denom = d00 * d11 - d01 * d01;

    // uvw is barycentric coords aka a mixture of the triangle vertices that averages out to the point we got
    double v = (d11 * d20 - d01 * d21) / denom;
    double w = (d00 * d21 - d01 * d20) / denom;
    double u = 1.0 - v - w;
    // we use that mixture with the triangle vertices that AREN'T in minkoski space to get the real contact point
    // std::cout << "We got " << glm::to_string((a[0] * u) + (b[0] * v) + (c[0] * w)) << " vs " << glm::to_string(projectedPoint);
    pointForObj1 = (a[1] * u) + (b[1] * v) + (c[1] * w);  
    pointForObj2 = (a[2] * u) + (b[2] * v) + (c[2] * w); 
            //std::cout << "Its either " << glm::to_string(pointForObj1) << " or " << glm::to_string(pointForObj2) << ".\n";
            //std::printf("Barycentric coords are %f %f %f \n", v, w, u);
            // std::printf("Considering support points %f %f %f and %f %f %f, and %f %f %f \n", a[2].x, a[2].y, a[2].z, b[2].x, b[2].y, b[2].z, c[2].x, c[2].y, c[2].z);

            //auto point = (a[2] * gamma) + (b[2] * beta) + (c[2] * alpha);
        // }


    // If this is a face-face collision, then, 
    // given our collision plane defined by pointForObj1 and minNormal, 
    // we can clip the faces against each other to find the vertices of the surface in contact
    auto m1 = dynamic_cast<ConvexTriangleMesh*>(collider1.physicsMesh->meshes.at(0).get());
    auto m2 = dynamic_cast<ConvexTriangleMesh*>(collider2.physicsMesh->meshes.at(0).get());
    if (m1 && m2) {
        glm::vec3 minNormalInModel1Space = invNorm1 * minNormal;
        // find face on m1 with the normal we got
        unsigned bestClippingFace = 0;
        float bestDot = glm::dot(minNormalInModel1Space, m1->faces[0].first);
        for (unsigned i = 1; i < m1->faces.size(); i++) {
            float dot = glm::dot(minNormalInModel1Space, m1->faces[i].first);
            //Assert(dot <= 1);
            if (dot > bestDot) {
                bestDot = dot;
                bestClippingFace = i;
            }
        }

        glm::dvec3 worldSpaceNormal = glm::normalize(glm::dvec3(norm1 * m1->faces[bestClippingFace].first));
        
        // Put this face in object2's model space
        glm::vec3 clippingFaceNormal = glm::normalize(invNorm2 * worldSpaceNormal);
        
        //DebugLogInfo("GOT MINNORMAL ", minNormal, " in model space ", minNormalInModelSpace, " converted to ", worldSpaceNormal, " in clip space ", clippingFaceNormal);

        
        std::vector<glm::vec3> clippingFace;
        for (auto& v : m1->faces[bestClippingFace].second) {
            clippingFace.push_back(glm::vec3(invWorld2 * world1 * glm::dvec4(v, 1.0)));
        }

        // Find side planes (point, normal) format
        std::vector<std::pair<glm::vec3, glm::vec3>> sidePlanes;
        for (unsigned i = 0; i < clippingFace.size(); i++) {
            auto v1 = clippingFace[i];
            auto v2 = clippingFace[(i + 1) % clippingFace.size()];
            auto sideNormal = glm::cross(v2 - v1, clippingFaceNormal);
            sidePlanes.push_back(std::make_pair(v1, glm::normalize(sideNormal)));
        }
        //sidePlanes.push_back(std::make_pair(clippingFace[0], clippingFaceNormal)); 

        // this is seemingly a face-face collision so we'll use the face closest to -clippingFaceNormal 
        unsigned bestContactFace = 0;
        bestDot = glm::dot(clippingFaceNormal, m1->faces[0].first);
        for (unsigned i = 1; i < m2->faces.size(); i++) {
            float dot = glm::dot(clippingFaceNormal, m1->faces[i].first);
            //Assert(dot >= -1);
            if (dot < bestDot) {
                bestDot = dot;
                bestContactFace = i;
            }
        }

        // Sutherland-Hodgman clipping algorithm 
        std::vector<glm::vec3> contactPointsInModel2Space = m2->faces[bestContactFace].second;
        for (auto& clippingPlane : sidePlanes) {
            std::vector<glm::vec3> input = contactPointsInModel2Space;
            //DebugLogInfo("INPUT ", input.size());
            contactPointsInModel2Space.clear();
            
            for (unsigned int i = 0; i < input.size(); i++) {
                // get edge
                auto v1 = input[i];
                auto v2 = input[(i + 1) % input.size()];
            
                // find intersection of edge v1v2 and the clippingPlane
                glm::vec3 intersectionPoint(NAN);
                glm::vec3 edgeDir = glm::vec3(v2 - v1);

                // do the clipping
                double distanceToV1 = SignedDistanceToPlane(clippingPlane.second, v1, clippingPlane.first);
                double distanceToV2 = SignedDistanceToPlane(clippingPlane.second, v2, clippingPlane.first);
                if (std::abs(glm::dot(clippingPlane.second, edgeDir)) > 0.0001) {
                    double t = (glm::dot(clippingPlane.first, clippingPlane.second) - glm::dot(clippingPlane.second, v1)) / glm::dot(clippingPlane.second, glm::normalize(edgeDir));
                    intersectionPoint = v1 + (glm::normalize(edgeDir) * t);
                    Assert(!std::isnan(intersectionPoint.x));
                }
                else {
                    if (distanceToV1 <= 0) {
                        contactPointsInModel2Space.push_back(v1);
                        //contactPointsInModel2Space.push_back(v2);
                    }
                    continue;
                }
                
            
                if (distanceToV1 <= 0) { // then v1 is on the right side of the side plane and should stay
                    contactPointsInModel2Space.push_back(v1);
                    //DebugLogInfo("V1 good");
                    if (distanceToV2 > 0) { // then v2 is on the wrong side of the plane and thus the edge v goes through the side plane, we should add the intersection point
                        contactPointsInModel2Space.push_back(intersectionPoint);
                        //DebugLogInfo("but V1-V2 crosses by ", distanceToV1);
                    }
                }  
                else if (distanceToV2 <= 0) { // then v1 is on wrong side and v2 is on right side, so we should add point of intersection (v2 itself will be added on next iteration)
                    contactPointsInModel2Space.push_back(intersectionPoint);
                    //DebugLogInfo("V1-V2 crosses back in by ", distanceToV2);
                }
                else {
                    //DebugLogInfo("nope");
                }
            } 
            
        }

        //DebugLogInfo("FINAL NCONTACTS ", contactPointsInModel2Space.size());


        std::vector<std::pair<glm::dvec3, double>> finalContactPoints;
        std::vector<std::pair<glm::dvec3, double>> otherFinalContactPoints;
        for (auto& v : contactPointsInModel2Space) {
            glm::dvec3 worldV = world2 * glm::dvec4(v, 1.0);
            double depth = -SignedDistanceToPlane(worldSpaceNormal, worldV, glm::dvec3(world1 * glm::dvec4(m1->faces[bestClippingFace].second[0], 1.0)));
            //double otherDepth = SignedDistanceToPlane(worldSpaceNormal, worldV, glm::dvec3(world2 * glm::dvec4(m2->faces[bestContactFace].second[0], 1.0)));
            //Assert(depth >= 0);
            if (depth > 0) {
                finalContactPoints.push_back(std::make_pair(worldV + glm::dvec3(worldSpaceNormal) * depth, depth));
                otherFinalContactPoints.push_back(std::make_pair(worldV - glm::dvec3(worldSpaceNormal) * depth, depth));
            }
        }

         if (finalContactPoints.size() > 0) {
            CollisionInfo info;
            info.collisionNormal = worldSpaceNormal;
            info.contactPoints = finalContactPoints;
            info.otherContactPoints = finalContactPoints;
            Assert(info.contactPoints.size() > 0);
            return info;
        }
    }

    CollisionInfo info;
    info.collisionNormal = minNormal,
    info.contactPoints = { std::make_pair(pointForObj1, minDistance), };
    info.otherContactPoints = { std::make_pair(pointForObj2, minDistance), };
return info;
}
 
struct CollidingSet {
    // a < b, always
    const TransformComponent* a;
    const TransformComponent* b;
   
    bool operator==(const CollidingSet&) const = default;

};

template <>
struct std::hash <CollidingSet> {
    std::size_t operator()(const CollidingSet& s) const noexcept {
        return (size_t)(s.a) ^ ((size_t)(s.b) << 1);
    }
};

// This cache is reset for every iteration of the physics engine so it's never wrong.
std::mutex collisionCacheMutex;
std::unordered_map<CollidingSet, std::optional<CollisionInfo>> collisionCache;

void ClearCollisionCache() {
    collisionCache.clear();
}

// this article actually does a really good job of explaining the GJK algorithm.
// https://cse442-17f.github.io/Gilbert-Johnson-Keerthi-Distance-Algorithm/
// TODO: concave support
// TODO optimizations for spheres
std::optional<CollisionInfo> IsColliding(
    const TransformComponent& transform1,
    const ColliderComponent& collider1,
    const TransformComponent& transform2,
    const ColliderComponent& collider2
) 
{
    CollidingSet pair = &transform1 < &transform2 ? CollidingSet{ .a = &transform1, .b = &transform2 } : CollidingSet{ .a = &transform2, .b = &transform1 };
    std::unique_lock l1(collisionCacheMutex);
    if (collisionCache.contains(pair)) {
        //auto collisioninfo = collisionCache[pair];
        //if (collisioninfo.has_value() && &transform1 > &transform2) {
            //collisioninfo->collisionNormal *= -1;
            //std::swap(collisioninfo->contactPoints, collisioninfo->otherContactPoints);
        //}
        //DebugLogInfo("CACHE HIT");
        //return collisioninfo;
    }
    else {
        //DebugLogInfo("CACHE MISS");
    }
    l1.unlock();

    const glm::mat3x3 invNormMatrix1 = glm::inverse(transform1.GetNormalMatrix());
    const glm::mat3x3 invNormMatrix2 = glm::inverse(transform2.GetNormalMatrix());
    //const glm::dmat4x4 invPhysMatrix1 = glm::inverse(transform1.GetPhysicsModelMatrix());
    const glm::dmat4x4 invPhysMatrix2 = glm::inverse(transform2.GetPhysicsModelMatrix());

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
    simplex.push_back(NewSimplexPoint(searchDirection, collider1, collider2, invNormMatrix1, invNormMatrix2, transform1.GetPhysicsModelMatrix(), transform2.GetPhysicsModelMatrix()));
    // std::cout << "INIT: Searched in " << glm::to_string(searchDirection) << " to get point " << glm::to_string(simplex.back()[0]) << "\n";
    // make new search direction go from simplex towards origin
    searchDirection = glm::normalize(-simplex.back()[0]);

    unsigned int nIterations = 0;
    while (true) {
        nIterations++;
        if (nIterations == 64) {
            DebugLogError("WARNING: GJK FAILED TO DETERMINE COLLISION AFTER 64 ITERATIONS. NANs likely.");
            std::unique_lock l2(collisionCacheMutex);
            collisionCache[pair] = std::nullopt;
            return std::nullopt;
        }

        // get new point for simplex
        auto newSimplexPoint = NewSimplexPoint(searchDirection, collider1, collider2, invNormMatrix1, invNormMatrix2, transform1.GetPhysicsModelMatrix(), transform2.GetPhysicsModelMatrix());

        // this is the farthest point in this direction, so if it didn't get past the origin, then origin is gonna be outside the minoski difference meaning no collision.
        if (glm::dot(newSimplexPoint[0], searchDirection) <= 0) {
            std::unique_lock l3(collisionCacheMutex);
            collisionCache[pair] = std::nullopt;
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

                auto collisioninfo = EPA(simplex, collider1, collider2, invNormMatrix1, invNormMatrix2, transform1.GetPhysicsModelMatrix(), transform2.GetPhysicsModelMatrix(), invPhysMatrix2, transform1.GetNormalMatrix());
                std::unique_lock l4(collisionCacheMutex);
                collisionCache[pair] = collisioninfo;
                if (&transform1 > &transform2) {
                    collisionCache[pair]->collisionNormal *= -1;
                    std::swap(collisionCache[pair]->contactPoints, collisionCache[pair]->otherContactPoints);
                }
                return collisioninfo;
                // std::cout << "THERE IS A COLLISION\n";
                // std::cout << "Positions are #1 = " << glm::to_string(transform1.Position()) << " and #2 = " << glm::to_string(transform2.Position()) << "\n";
                //auto result = FindContact(transform1, collider1, transform2, collider2);
                //if (!result) {
                    //std::cout << "SAT and GJK disagreed, uh oh.\n";
                //}
                //return result;
            }
            break;
            default:
            std::cout << "GJK: WHAT\n";
            abort();
            break;
        }
    }
}