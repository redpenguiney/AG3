#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/component_registry.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <tuple>
#include <vector>

void SpatialAccelerationStructure::Update() {
    //auto start = Time();
    // Get components of all gameobjects that have a transform and collider component
    auto components = ComponentRegistry::GetSystemComponents<TransformComponent, ColliderComponent>();

    for (auto & tuple: components) {
        auto& colliderComp = *std::get<1>(tuple);
        auto& transformComp = *std::get<0>(tuple);
        if (colliderComp.live) {
            if (transformComp.moved) {
                transformComp.moved = false;
                // std::cout << "Updating collider " << colliderComp << " at i=" << i <<", j=" << j << ".\n";
                UpdateCollider(colliderComp, transformComp);
                
            } 
        }      
    }

    //LogElapsed(start, "\nSAS update elapsed ");
}

void SpatialAccelerationStructure::AddIntersectingLeafNodes(SpatialAccelerationStructure::SasNode* node, std::vector<SpatialAccelerationStructure::SasNode*>& collidingNodes, const AABB& collider) {
    if (node->aabb.TestIntersection(collider)) { // if this node touched the given collider, then its children may as well.
        if (node->children != nullptr) {
            for (auto& child : *node->children) {
                AddIntersectingLeafNodes(child, collidingNodes, collider);
            } 
        }
        
        if (!node->objects.empty()) {
            collidingNodes.push_back(node);
        }
    }
}

void SpatialAccelerationStructure::AddIntersectingLeafNodes(SpatialAccelerationStructure::SasNode* node, std::vector<SpatialAccelerationStructure::SasNode*>& collidingNodes, const glm::dvec3& origin, const glm::dvec3& inverse_direction) {
    if (node->aabb.TestIntersection(origin, inverse_direction)) { // if this node touched the given collider, then its children may as well.
        if (node->children != nullptr) {
            //std::cout << "hay " << node->children->size() << "\n";
            for (auto& child : *(node->children)) {
                AddIntersectingLeafNodes(child, collidingNodes, origin, inverse_direction);
            } 
        }
        
        if (!node->objects.empty()) {
            collidingNodes.push_back(node);
        }
    }
}

std::vector<SpatialAccelerationStructure::ColliderComponent*> SpatialAccelerationStructure::Query(const AABB& collider) {
    // find leaf nodes whose AABBs intersect the collider
    std::vector<SpatialAccelerationStructure::SasNode*> collidingNodes;
    AddIntersectingLeafNodes(&root, collidingNodes, collider);
    
    // test the aabbs of the objects inside each node and if so add them to the vector
    std::vector<SpatialAccelerationStructure::ColliderComponent*> collidingComponents;
    for (auto & node: collidingNodes) {
        for (auto & obj: node->objects) {
            if (obj->aabb.TestIntersection(collider)) {
                collidingComponents.push_back(obj);
            }
        }
        if (node->objects.size() > NODE_SPLIT_THRESHOLD) {
            node->Split();
        }
    }

    return collidingComponents;
}

// TODO: redundant code in these two query functions, could improve
std::vector<SpatialAccelerationStructure::ColliderComponent*> SpatialAccelerationStructure::Query(const glm::dvec3& origin, const glm::dvec3& direction) {
    glm::dvec3 inverse_direction = glm::dvec3(1.0/direction.x, 1.0/direction.y, 1.0/direction.z); 

    // find leaf nodes whose AABBs intersect the ray
    std::vector<SpatialAccelerationStructure::SasNode*> collidingNodes;
    AddIntersectingLeafNodes(&root, collidingNodes, origin, inverse_direction);

    // test the aabbs of the objects inside each node and if so add them to the vector
    std::vector<SpatialAccelerationStructure::ColliderComponent*> collidingComponents;
    //std::cout << "For raycast, testing " << collidingNodes.size() << " nodes.\n";
    for (auto & node: collidingNodes) {
        //std::cout << "\twithin this node testing " << node->objects.size() << " collider AABBs.\n";
        for (auto & obj: node->objects) {
            if (obj->aabb.TestIntersection(origin, inverse_direction)) {
                collidingComponents.push_back(obj);
            }
        }
        if (node->objects.size() > NODE_SPLIT_THRESHOLD) {
            node->Split();
            std::cout << "WE GOTTA SPLIT\n";
        }
    }

    return collidingComponents;
}

void SpatialAccelerationStructure::DebugVisualizeAddVertexAttributes(SasNode const& node, std::vector<float>& instancedVertexAttributes, unsigned int& numInstances) {
    if (node.children != nullptr) {
        for (auto & child: *node.children) {
            if (child != nullptr) {
                DebugVisualizeAddVertexAttributes(*child, instancedVertexAttributes, numInstances);
            }
        }
    }
    
    for (auto & object: node.objects) {
        glm::vec3 position = object->aabb.Center();
        glm::mat4x4 model = glm::translate(glm::identity<glm::mat4x4>(), position);
        instancedVertexAttributes.resize(instancedVertexAttributes.size() + 16);
        memcpy(instancedVertexAttributes.data() + instancedVertexAttributes.size() - 16, &model, sizeof(glm::mat4x4));
        instancedVertexAttributes.push_back(1);
        instancedVertexAttributes.push_back(1);
        instancedVertexAttributes.push_back(0);
        instancedVertexAttributes.push_back(1);
        numInstances++;
    }

    glm::vec3 position = node.aabb.Center();
    glm::mat4x4 model = glm::translate(glm::scale(glm::identity<glm::mat4x4>(), glm::vec3(node.aabb.max - node.aabb.min)), position);
    instancedVertexAttributes.resize(instancedVertexAttributes.size() + 16);
    memcpy(instancedVertexAttributes.data() + instancedVertexAttributes.size() - 16, &model, sizeof(glm::mat4x4));
    instancedVertexAttributes.push_back(0);
    instancedVertexAttributes.push_back(1);
    instancedVertexAttributes.push_back(1);
    instancedVertexAttributes.push_back(1);
    numInstances++;
}

void SpatialAccelerationStructure::DebugVisualize() {
    static auto crummyDebugShader =  ShaderProgram::New("../shaders/debug_simple_vertex.glsl", "../shaders/debug_simple_fragment.glsl", {}, false, true, false);
    crummyDebugShader->Use();

    const static auto m = Mesh::FromFile("../models/rainbowcube.obj", true, true);
    const static auto& vertices = m->vertices; // remember, its xyz, uv, normal, tangent tho we only bothering with xyz
    const static auto& indices = m->indices;

    
    std::vector<float> instancedVertexAttributes; // per object data. format is 4x4 model mat, rgba, 4x4 model mat, rgba...
    unsigned int numInstances; // number of wireframes we're drawing
    DebugVisualizeAddVertexAttributes(root, instancedVertexAttributes, numInstances);

    GLuint vao, vbo, ibo, ivbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ivbo);
    glGenBuffers(1, &ibo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STREAM_DRAW);    
    glBindBuffer(GL_ARRAY_BUFFER, ivbo);
    glBufferData(GL_ARRAY_BUFFER, instancedVertexAttributes.size() * sizeof(GLfloat), instancedVertexAttributes.data(), GL_STREAM_DRAW);    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STREAM_DRAW);  


    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0); // position

    glVertexAttribPointer(0, 3, GL_FLOAT, false, m->vertexSize, (void*)0);

    glVertexAttribDivisor(0, 0); // tell openGL that vertex position is per-vertex


    glBindBuffer(GL_ARRAY_BUFFER, ivbo);

    glEnableVertexAttribArray(2); // model matrix (needs 4 vec4 attributes)
    glEnableVertexAttribArray(2+1);
    glEnableVertexAttribArray(2+2);
    glEnableVertexAttribArray(2+3);
    glEnableVertexAttribArray(1); // color

    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)0);
    glVertexAttribPointer(2+1, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)(sizeof(glm::vec4) * 1));
    glVertexAttribPointer(2+2, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)(sizeof(glm::vec4) * 2));
    glVertexAttribPointer(2+3, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)(sizeof(glm::vec4) * 3));
    glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)sizeof(glm::mat4x4));

    glVertexAttribDivisor(2, 1); // tell openGL that these are instanced vertex attributes (meaning per object, not per vertex)
    glVertexAttribDivisor(2+1, 1);
    glVertexAttribDivisor(2+2, 1);
    glVertexAttribDivisor(2+3, 1);
    glVertexAttribDivisor(1, 1);

    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr, numInstances);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDeleteBuffers(1, &vbo);  
    glDeleteBuffers(1, &ivbo);  
    glDeleteBuffers(1, &ibo);  
    
    glDeleteVertexArrays(1, &vao);
}


void SpatialAccelerationStructure::SasNode::Split() {
    std::cout << "splitting.\n";

    assert(objects.size() >= NODE_SPLIT_THRESHOLD);
    assert(!split);
    glm::dvec3 meanPosition = {0, 0, 0};
    for (auto & obj : objects) {
        meanPosition += obj->aabb.Center()/(double)objects.size();
    }
    splitPoint = meanPosition;

    children = new std::array<SasNode*, 27> {nullptr};
    for (int x = -1; x < 2; x++) {
        for (int y = -1; y < 2; y++) {
            for (int z = -1; z < 2; z++) {
                auto node = new SasNode();
                node->parent = this;
                (*children).at((x + 1) * 9 + (y + 1) * 3 + z + 1) = node;
            }
        }
    }

    std::vector<unsigned int> indicesToRemove;
    unsigned int i = 0;
    for (auto & obj: objects) { 
        auto index = SasInsertHeuristic(*this, obj->aabb);
        if (index != -1) {
            //std::cout << "aoisjfoijesa " << children << " index " << index << "\n";
            (*children).at(index)->objects.push_back(obj);   
            indicesToRemove.push_back(i);
        }
        i++;
    }
    
    for (auto & node: *children) {
        node->CalculateAABB();
    }

    // iterate backwards through indicesToRemove to preserve index correctness
    if (indicesToRemove.size() > 0) {
        for (unsigned int j = indicesToRemove.size() - 1; j > 0; j--) {
            //std::cout << objects.size() << " objects, i = "<< i << " index " << indicesToRemove[i] << "\n";
            assert(indicesToRemove[j] < objects.size());
            objects.erase(objects.begin() + indicesToRemove[j]);
        }
    }
}

void SpatialAccelerationStructure::SasNode::CalculateAABB() {
    // first find a single object or child node that has an aabb, and use that as a starting point
    if (objects.size() > 0) {
        aabb = objects[0]->aabb;
    }
    else if (children != nullptr) {
        bool found = false;
        for (auto & child: *children) {
            if (child != nullptr) {
                aabb = child->aabb;
                found = true;
                break;
            }
        }
        assert(found);
    }

    // now aabb is within the real aabb, we get to real aabb by growing aabb to contain every child node and object
    for (auto & obj: objects) {
        aabb.Grow(obj->aabb);
    }
    if (children != nullptr) {
        for (auto & child: *children) {
            if (child != nullptr) {
                aabb.Grow(child->aabb);
            }
        }
    }

}

SpatialAccelerationStructure::SasNode::SasNode() {
    split = false;
    splitPoint = {NAN, NAN, NAN};
    parent = nullptr;
    children = nullptr;
}


int SpatialAccelerationStructure::SasInsertHeuristic(const SpatialAccelerationStructure::SasNode& node, const AABB& aabb) {
    if (node.children == nullptr) {return -1;}
    auto splitPoint = node.splitPoint;
    unsigned int x = 1, y = 1, z = 1;
    if (aabb.min.x > splitPoint.x) {
        x += 1;
    } 
    if (aabb.max.x < splitPoint.x) {
        x -= 1;
    }
    if (aabb.min.y > splitPoint.y) {
        y += 1;
    } 
    if (aabb.max.y < splitPoint.y) {
        y -= 1;
    }
    if (aabb.min.z > splitPoint.z) {
        z += 1;
    } 
    if (aabb.max.z < splitPoint.z) {
        z -= 1;
    }

    return x * 9 + y * 3 + z;
}  

void SpatialAccelerationStructure::AddCollider(ColliderComponent* collider, const TransformComponent& transform) {
    collider->RecalculateAABB(transform);

    // Recursively pick best child node starting from root until we reach leaf node
    SasNode* currentNode = &root;
    while (true) {
        int childIndex = SasInsertHeuristic(*currentNode, collider->aabb);
        if (childIndex == -1) {
            break;
        }
        else {
            currentNode = currentNode->children->at(childIndex);
        }
    }

    // put collider in that leaf node
    currentNode->objects.push_back(collider);
    currentNode->split = false;
    collider->node = currentNode;

    // Expand node and its ancestors to make sure they contain collider
    while (true) {
        currentNode->aabb.Grow(collider->aabb);
        if (currentNode->parent == nullptr) {
            return;
        }
        else {
            currentNode = currentNode->parent;
        }
    }
}

void SpatialAccelerationStructure::ColliderComponent::RemoveFromSas() {
    // remove object from node
    for (unsigned int i = 0; i < node->objects.size(); i++) {
        if (node->objects[i] == this) {
            node->objects.erase(node->objects.begin() + i);
            break;
        }
    }

    // TODO: we need to do something when node is emptied, probably    
}

void SpatialAccelerationStructure::UpdateCollider(SpatialAccelerationStructure::ColliderComponent& collider, const TransformComponent& transform) {
    collider.RecalculateAABB(transform);
    const AABB& newAabb = collider.aabb;

    // Go up the tree from the collider's current node to find the first node that fully envelopes the collider.
    // (if collider's current node still envelops the collider, this will do nothing)
    
    SasNode* oldNode = collider.node;
    SasNode* smallestNodeThatEnvelopes = collider.node;
    while (true) {
        if (!smallestNodeThatEnvelopes->aabb.TestEnvelopes(newAabb)) {
            // If we reach the root and it doesn't fit, it will grow the root node's aabb to contain the collider.
            if (smallestNodeThatEnvelopes->parent == nullptr) {
                smallestNodeThatEnvelopes->aabb.Grow(newAabb);
                break; //obvi not gonna be any more parent nodes after this
            }
            else {
                smallestNodeThatEnvelopes = smallestNodeThatEnvelopes->parent;
            }
        }
        else {
            // we found a node that contains the collider
            break;
        }
    }

    // if the node still fits don't do anything
    if (oldNode == smallestNodeThatEnvelopes) {
        return;
    }

    // remove collider from old node
    for (unsigned int i = 0; i < oldNode->objects.size(); i++) { // TODO: O(n) time here could actually be an issue
        if (oldNode->objects[i] == &collider) {
            oldNode->objects.erase(oldNode->objects.begin() + i);
            break;
        }
    }

    // Then, we use the insert heuristic to go down child nodes until we find the best leaf node for the collider
    SasNode* newNodeForCollider = smallestNodeThatEnvelopes;
    while (true) {
        int childIndex = SasInsertHeuristic(*newNodeForCollider, newAabb);
        // TODO: we might need to grow this 
        if (childIndex == -1) { // then we're at a leaf node, add the collider to it
            collider.node = newNodeForCollider;
            newNodeForCollider->objects.push_back(&collider);
            return;
        }
        else {
            newNodeForCollider = newNodeForCollider->children->at(childIndex);
        }
    }
}

SpatialAccelerationStructure::SpatialAccelerationStructure() {
    root = SasNode();
}

SpatialAccelerationStructure::~SpatialAccelerationStructure() {

}

void SpatialAccelerationStructure::ColliderComponent::Init(GameObject* gameobj, std::shared_ptr<PhysicsMesh>& physMesh) {
    aabbType = AABBBoundingCube;
    node = nullptr;
    gameobject = gameobj;
    physicsMesh = physMesh;
    SpatialAccelerationStructure::Get().AddCollider(this, *gameobject->transformComponent);
}

void SpatialAccelerationStructure::ColliderComponent::Destroy() {
    physicsMesh = nullptr;
}

std::shared_ptr<GameObject>& SpatialAccelerationStructure::ColliderComponent::GetGameObject() {
    return ComponentRegistry::GAMEOBJECTS[gameobject];
}

// TODO: collider AABBs should be augmented to contain their motion over the next time increment.
    // If we ever use a second SAS for accelerating visibility queries too, then don't do it for that
void SpatialAccelerationStructure::ColliderComponent::RecalculateAABB(const TransformComponent& colliderTransform) {
    // std::cout << "Reacalculating AABB of " << this << "\n";
    if (aabbType == AABBBoundingCube) {
        glm::dvec3 min = {-std::sqrt(0.75), -std::sqrt(0.75), -std::sqrt(0.75)};
        glm::dvec3 max = {std::sqrt(0.75), std::sqrt(0.75), std::sqrt(0.75)};
        
        min *= AABB_FAT_FACTOR;
        min *= colliderTransform.scale();
        max *= AABB_FAT_FACTOR;
        max *= colliderTransform.scale();

        min += colliderTransform.position();
        max += colliderTransform.position();
        aabb = AABB(min, max);
    }
    else {
        std::printf("PROBLEM\n");
        abort();
    }
}

const AABB& SpatialAccelerationStructure::ColliderComponent::GetAABB() {
    return aabb;
}