#include "spatial_acceleration_structure.hpp"
#include "gameobjects/gameobject.hpp"
#include "glm/gtx/string_cast.hpp"
#include "graphics/gengine.hpp"
#include "gameobjects/collider_component.hpp"

void SpatialAccelerationStructure::Update() {
    //auto start = Time();
    // Get components of all gameobjects that have a transform and collider component

    for (auto it = GameObject::SystemGetComponents<TransformComponent, ColliderComponent>({ ComponentBitIndex::Transform, ComponentBitIndex::Collider });  it.Valid(); it++) {
        auto & tuple = *it;
        auto& colliderComp = *std::get<1>(tuple);
        auto& transformComp = *std::get<0>(tuple);
        if (transformComp.moved) {
            transformComp.moved = false;
            // std::cout << "Updating collider " << &colliderComp << "\n";
            UpdateCollider(colliderComp, transformComp);
                
        }      
    }

    //LogElapsed(start, "\nSAS update elapsed ");
}

void SpatialAccelerationStructure::AddIntersectingLeafNodes(SpatialAccelerationStructure::SasNode* node, std::vector<SpatialAccelerationStructure::SasNode*>& collidingNodes, const AABB& collider, CollisionLayerSet layers) {
    if ((node->layers & layers).any() && node->aabb.TestIntersection(collider)) { // if this node touched the given collider, then its children may as well.
        if (node->children != nullptr) {
            for (auto& child : *node->children) {
                AddIntersectingLeafNodes(child, collidingNodes, collider, layers);
            } 
        }
        
        if (!node->objects.empty()) {
            collidingNodes.push_back(node);
        }
    }
}

void SpatialAccelerationStructure::AddIntersectingLeafNodes(SpatialAccelerationStructure::SasNode* node, std::vector<SpatialAccelerationStructure::SasNode*>& collidingNodes, const glm::dvec3& origin, const glm::dvec3& inverse_direction, CollisionLayerSet layers) {
    if ((node->layers & layers).any() && node->aabb.TestIntersection(origin, inverse_direction)) { // if this node touched the given collider, then its children may as well.
        if (node->children != nullptr) {
            //std::cout << "hay " << node->children->size() << "\n";
            for (auto& child : *(node->children)) {
                AddIntersectingLeafNodes(child, collidingNodes, origin, inverse_direction, layers);
            } 
        }
        
        if (!node->objects.empty()) {
            collidingNodes.push_back(node);
        }
    }
}

std::vector<ColliderComponent*> SpatialAccelerationStructure::Query(const AABB& collider, CollisionLayerSet layers) {
    // find leaf nodes whose AABBs intersect the collider
    std::vector<SpatialAccelerationStructure::SasNode*> collidingNodes;
    AddIntersectingLeafNodes(&root, collidingNodes, collider, layers);
    
    // test the aabbs of the objects inside each node and if so add them to the vector
    std::vector<ColliderComponent*> collidingComponents;
    for (auto & node: collidingNodes) {
        for (auto & obj: node->objects) {
            if (layers[obj->layer] == true && obj->aabb.TestIntersection(collider)) {
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
std::vector<ColliderComponent*> SpatialAccelerationStructure::Query(const glm::dvec3& origin, const glm::dvec3& direction, CollisionLayerSet layers) {
    glm::dvec3 inverse_direction = glm::dvec3(1.0/direction.x, 1.0/direction.y, 1.0/direction.z); 

    // find leaf nodes whose AABBs intersect the ray
    std::vector<SpatialAccelerationStructure::SasNode*> collidingNodes;
    AddIntersectingLeafNodes(&root, collidingNodes, origin, inverse_direction, layers);

    // test the aabbs of the objects inside each node and if so add them to the vector
    std::vector<ColliderComponent*> collidingComponents;
    for (auto & node: collidingNodes) {
        for (auto & obj: node->objects) {
            if (layers[obj->layer] && obj->aabb.TestIntersection(origin, inverse_direction)) {
                collidingComponents.push_back(obj);
            }
        }
        if (node->objects.size() > NODE_SPLIT_THRESHOLD) {
            node->Split();
        }
    }

    return collidingComponents;
}

void SpatialAccelerationStructure::DebugVisualizeAddVertexAttributes(SasNode const& node, std::vector<float>& instancedVertexAttributes, unsigned int& numInstances, const Mesh& mesh, unsigned int depth) {
    if (node.children != nullptr) {
        for (auto & child: *node.children) {
            if (child != nullptr) {
                DebugVisualizeAddVertexAttributes(*child, instancedVertexAttributes, numInstances, mesh, depth + 1);
            }
        }
    }
    
    for (auto & object: node.objects) {
        instancedVertexAttributes.resize(instancedVertexAttributes.size() + mesh.vertexFormat.GetInstancedVertexSize()/sizeof(GLfloat));
        auto oldPtr = instancedVertexAttributes.data() + instancedVertexAttributes.size() - mesh.vertexFormat.GetInstancedVertexSize() / sizeof(GLfloat); // gotta set it afterwards bc resizing changes ptr

        glm::vec3 position = object->aabb.Center();
        glm::mat4x4 model = glm::scale(glm::translate(glm::identity<glm::mat4x4>(), position), glm::vec3(object->aabb.max - object->aabb.min)) ;
        glm::vec4 colors[MAX_COLLISION_LAYERS] = {
            {1, 1, 1, 1},
            { 1, 0, 0, 1 },
            { 1, 1, 0, 1 },
            { 1, 0, 1, 1 },
            { 0, 1, 1, 1 },
            { 0, 0, 1, 1 },
            { 0, 0, 0, 1 },
            { 0, 1, 0, 1 },
            { 0.5, 0.5, 0.5, 1 },
            { 1, 0.5, 0.5, 1 },
            { 1, 1, 0.5, 1 },
            { 1, 0.5, 1, 1 },
            { 0.5, 1, 1, 1 },
            { 0.5, 0.5, 1, 1 },
            { 0.5, 0.5, 0.5, 1 },
            { 0.5, 1, 0.5, 1 },
        };
        memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &colors[object->layer], sizeof(glm::vec4));
        memcpy(oldPtr + mesh.vertexFormat.attributes.modelMatrix->offset/sizeof(GLfloat), &model, sizeof(glm::mat4x4));
       /* if (depth == 0) {
            constexpr glm::vec4 color = { 1, 1, 1, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        else if (depth == 1) {
            constexpr glm::vec4 color = { 1, 0, 0, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        else if (depth == 2) {
            constexpr glm::vec4 color = { 0, 1, 1, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        else {
            constexpr glm::vec4 color = { 1, 0, 1, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }*/
        numInstances++;
    }

    //if (&node != &root) {
        instancedVertexAttributes.resize(instancedVertexAttributes.size() + mesh.vertexFormat.GetInstancedVertexSize() / sizeof(GLfloat));
        auto oldPtr = instancedVertexAttributes.data() + instancedVertexAttributes.size() - mesh.vertexFormat.GetInstancedVertexSize() / sizeof(GLfloat); // gotta set it afterwards bc resizing changes ptr;

        glm::vec3 position = node.aabb.Center();
        glm::mat4x4 model = glm::scale(glm::translate(glm::identity<glm::mat4x4>(), position), glm::vec3(node.aabb.max - node.aabb.min)) ;

        memcpy(oldPtr + mesh.vertexFormat.attributes.modelMatrix->offset / sizeof(GLfloat), &model, sizeof(glm::mat4x4));
        if (depth == 0) {
            constexpr glm::vec4 color = { 1, 1, 1, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        else if (depth == 1) {
            constexpr glm::vec4 color = { 1, 0, 0, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        else if (depth == 2) {
            constexpr glm::vec4 color = { 0, 1, 1, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        else {
            constexpr glm::vec4 color = { 1, 0, 1, 1 };
            memcpy(oldPtr + mesh.vertexFormat.attributes.color->offset / sizeof(GLfloat), &color, sizeof(glm::vec4));
        }
        
        numInstances++;
    //}
}
    

void SpatialAccelerationStructure::DebugVisualize() {
    static auto crummyDebugShader = ShaderProgram::New("../shaders/debug_simple_vertex.glsl", "../shaders/debug_simple_fragment.glsl", false, false);

    crummyDebugShader->Use();

    const auto m = Mesh::MultiFromFile("../models/rainbowcube.obj", MeshCreateParams()).back().mesh;
    const auto& vertices = m->vertices; // remember, its xyz, uv, normal, tangent tho we only bothering with xyz
    const auto& indices = m->indices;

    
    std::vector<float> instancedVertexAttributes; // per object data. format is 4x4 model mat, rgba, 4x4 model mat, rgba...
    unsigned int numInstances = 0; // number of wireframes we're drawing
    DebugVisualizeAddVertexAttributes(root, instancedVertexAttributes, numInstances, *m);

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

    m->vertexFormat.SetNonInstancedVaoVertexAttributes(vao, m->instancedVertexSize, m->nonInstancedVertexSize);


    glBindBuffer(GL_ARRAY_BUFFER, ivbo);

    m->vertexFormat.SetInstancedVaoVertexAttributes(vao, m->instancedVertexSize, m->nonInstancedVertexSize);

    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glViewport(0, 0, GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height);
    glDisable(GL_SCISSOR_TEST);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr, numInstances);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDeleteBuffers(1, &vbo);  
    glDeleteBuffers(1, &ivbo);  
    glDeleteBuffers(1, &ibo);  
    
    glDeleteVertexArrays(1, &vao);
}

// URGENT TODO: make sure not all objects put in same child node recursively
void SpatialAccelerationStructure::SasNode::Split() {
    DebugLogInfo("splitting."); // keep this here so i know what happened when the above TODO i'll never do causes problems

    Assert(objects.size() >= NODE_SPLIT_THRESHOLD);
    // unsigned int countbefore = objects.size();

    // std::cout << "Before split, node has " << objects.size() << " objects\n";
    Assert(!split);

    // calculate split point
    glm::dvec3 meanPosition = {0, 0, 0};
    for (auto & obj : objects) {
        meanPosition += obj->aabb.Center()/(double)objects.size();
    }
    splitPoint = meanPosition;

    // create child nodes
    children = new std::array<SasNode*, 27> {nullptr};
    for (int x = -1; x < 2; x++) {
        for (int y = -1; y < 2; y++) {
            for (int z = -1; z < 2; z++) {
                auto node = new SasNode();
                node->parent = this;
                (*children).at((x + 1) * 9 + (y + 1) * 3 + z + 1) = node;
                Assert(node->objects.size() == 0);
            }
        }
    }

    

    // determine which child nodes get which objects
    std::vector<unsigned int> indicesToRemove; // we cant remove the objects while we're iterating cuz that would mess up the iterator
    unsigned int i = 0;
    for (auto & obj: objects) { 
        auto node = SasInsertHeuristic(*this, obj->aabb, obj->layer);
        if (node != nullptr && node != this) {
            //std::cout << "aoisjfoijesa " << children << " index " << index << "\n";
            node->objects.push_back(obj);   
            obj->node = node;
            indicesToRemove.push_back(i);
            //std::cout << "pushing " << i << " \n";
        }
        i++;
    }

    // determine child node AABBs
    for (auto & node: *children) {
        node->RecalculateAABB();
    }

    //std::cout << indicesToRemove.size() << " is the size\n";

    // iterate backwards through indicesToRemove to preserve index correctness
    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it ) {
        Assert(*it < objects.size());
        objects.erase(objects.begin() + *it);
    }

    // unsigned int countafter = objects.size();
    // for (auto & node: *children) {
    //     countafter += node->objects.size();
    //     std::cout << "\tThis node has " << node->objects.size() << "\n";
    // }
    // std::cout << "After split, node has " << countafter << " objects\n";

    // Assert(countbefore == countafter);
}

void SpatialAccelerationStructure::SasNode::RecalculateAABB() {
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
        Assert(found);
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


    // if (parent) { // if node gets bigger/smaller its parent should adjust accordingly
    //     parent->CalculateAABB(); 
    // }

    
}

SpatialAccelerationStructure::SasNode::SasNode() {
    split = false;
    splitPoint = {NAN, NAN, NAN};
    parent = nullptr;
    children = nullptr;
}


SpatialAccelerationStructure::SasNode* SpatialAccelerationStructure::SasInsertHeuristic(SasNode& node, const AABB& aabb, CollisionLayer layer) {
    if (node.children == nullptr) {return nullptr;}
    if (node.aabb.Volume() < aabb.Volume() * 8) {return &node;}
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

    Assert(&node != (*node.children)[x * 9 + y * 3 + z]);
    return (*node.children)[x * 9 + y * 3 + z];
}  

void SpatialAccelerationStructure::AddCollider(ColliderComponent* collider, const TransformComponent& transform) {
    collider->RecalculateAABB(transform);

    // Recursively pick best child node starting from root until we reach leaf node
    SasNode* currentNode = &root;
    while (true) {
        currentNode->layers.set(collider->layer, true);
        auto childNode = SasInsertHeuristic(*currentNode, collider->aabb, collider->layer);
        if (childNode == nullptr) {
            break;
        }
        else {
            currentNode = childNode;
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

void ColliderComponent::RemoveFromSas() {
    //DebugLogInfo("REMOVING");
    bool layerExists = false; // we need to see if this is the only object with the given layer in this node 
    // remove object from node
    for (unsigned int i = 0; i < node->objects.size(); i++) { // todo: could maybe optimize this loop?
        //DebugLogInfo("layer ", layer, " vs ", node->objects[i]->layer, " (", i, ")", " comp = ", layer == node->objects[i]->layer);
        if (node->objects[i] == this) {
            node->objects.erase(node->objects.begin() + i);
            i--;
        }
        else if (layer == node->objects[i]->layer) {
            //DebugLogInfo("YO");
            layerExists = true;
        }
        //else {
            //Assert(layer != node->objects[i]->layer);
        //}
    }

    if (!layerExists) {
        node->layers.set(layer, false);
        auto currentNode = node;
        while (currentNode->parent) {
            currentNode = currentNode->parent;
            for (auto& n : *(currentNode->children)) {
                if (n->layers[layer]) {
                    goto done; // escape outer loop
                }
            }
            currentNode->layers.set(layer, false);
        }
    }
    done:;

    // TODO: we need to do something when node is emptied, probably   
}

void SpatialAccelerationStructure::UpdateCollider(ColliderComponent& collider, const TransformComponent& transform) {
    collider.RecalculateAABB(transform);
    const AABB& newAabb = collider.aabb;

    // Go up the tree from the collider's current node to find the first node that fully envelopes the collider.
    // (if collider's current node still envelops the collider, this will do nothing)
    
    //std::cout << " root node at " << &root << "\n";
    SasNode* const oldNode = collider.node;
    SasNode* newNodeForCollider = collider.node;
    while (true) {
        //std::cout << " current node is " << newNodeForCollider << ", parent is " << newNodeForCollider->parent << "\n";
        if (!newNodeForCollider->aabb.TestEnvelopes(newAabb)) {
            //std::cout << "current node doesn't fit\n";
            // If we reach the root and it doesn't fit, it will grow the root node's aabb to contain the collider.
            if (newNodeForCollider->parent == nullptr) {
                //std::cout << "...smallest enveloping node is root\n";
                newNodeForCollider->aabb.Grow(newAabb);
                break; //obvi not gonna be any more parent nodes after this
            }
            else {
                newNodeForCollider = newNodeForCollider->parent;
            }
        }
        else {
            //std::cout << " we win\n";
            // we found a node that contains the collider
            break;
        }

    }

    //std::cout << "smallest enveloping ancestor is "  << newNodeForCollider << "\n";
    //std::cout << "find children.\n";
    // Then, we use the insert heuristic to go down child nodes until we find the best leaf node for the collider
    while (true) {
        auto childNode = SasInsertHeuristic(*newNodeForCollider, newAabb, collider.layer);
        //std::cout << "\tchild is " << childNode << "\n.";
        // TODO: we might need to grow this 
        if (childNode == nullptr || childNode == newNodeForCollider) { // then we're at a leaf node, add the collider to it
            collider.node = newNodeForCollider;

            break;
        }
        else {
            newNodeForCollider = childNode;
        }

    }

    //std::cout << "selected child "  << newNodeForCollider << "\n";

    //std::cout << "expanding nodes.\n";
    // Expand node and its ancestors to make sure they fully contain collider (faster than doing recursive RecalculateAABB())
    auto p1 = newNodeForCollider;
    while (true) {
        p1->aabb.Grow(newAabb);
        if (p1->parent == nullptr) {
            break;
        }
        else {
            p1 = p1->parent;
        }
    }


    
    // if the object is in the same node after all that, we don't need to update parents/children
    if (oldNode == newNodeForCollider) {
        //std::cout << "done.\n";
        return;
    }
    else {
        //std::cout << "not done.\n";
        // add collider to new node
        newNodeForCollider->objects.push_back(&collider);

        // remove collider from old node
        for (unsigned int i = 0; i < oldNode->objects.size(); i++) { // TODO: O(n) time here could actually be an issue
            if (oldNode->objects[i] == &collider) {
                oldNode->objects.erase(oldNode->objects.begin() + i);
            }
        }

        // with that collider removed, old node (and by extension its parents) might be able to be made tighter
        oldNode->RecalculateAABB();
        auto p = oldNode;
        while (true) {
            if (p->parent) {
                p = p->parent;
                p->RecalculateAABB();
            }
            else {
                return;
            }
        }
        return;
    }

    
}

void SpatialAccelerationStructure::UpdateColliderLayer(ColliderComponent& collider, CollisionLayer oldLayer)
{
    auto p = collider.node;
    while (p && p->layers[collider.layer] == false) {
        p->layers.set(collider.layer, true);
        p = p->parent;
    }

    bool layerExists = false; // we need to see if this was the only object with the given layer in this node, and if so we need to remove that layer from the node itself (and possibly its parents too)
    for (unsigned int i = 0; i < collider.node->objects.size(); i++) { // todo: could maybe optimize this loop?
        if (oldLayer == collider.node->objects[i]->layer) {
            layerExists = true;
        }
    }

    if (!layerExists) {
        collider.node->layers.set(oldLayer, false);
        auto currentNode = collider.node;
        while (currentNode->parent) {
            currentNode = currentNode->parent;
            for (auto& n : *(currentNode->children)) {
                if (n->layers[oldLayer]) {
                    return;
                }
            }
            currentNode->layers.set(oldLayer, false);
        }
    }
}

SpatialAccelerationStructure::SpatialAccelerationStructure() {
    root = SasNode();
}

SpatialAccelerationStructure::~SpatialAccelerationStructure() {

}


#ifdef IS_MODULE
SpatialAccelerationStructure* _SPATIAL_ACCELERATION_STRUCTURE_ = nullptr;
void SpatialAccelerationStructure::SetModuleSpatialAccelerationStructure(SpatialAccelerationStructure* structure) {
    _SPATIAL_ACCELERATION_STRUCTURE_ = structure;
}
#endif

SpatialAccelerationStructure& SpatialAccelerationStructure::Get() {
    #ifdef IS_MODULE
    Assert(_SPATIAL_ACCELERATION_STRUCTURE_ != nullptr);
    return *_SPATIAL_ACCELERATION_STRUCTURE_;
    #else
    static SpatialAccelerationStructure structure;
    return structure;
    #endif
}