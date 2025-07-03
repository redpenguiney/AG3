#include "GLM/gtx/string_cast.hpp"
#include "mesh.hpp"
#include "material.hpp"
//#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "gengine.hpp"

//#pragma comment(lib, "assimp-vc143-mtd.lib")

glm::mat4x4 AssimpMatrixToGLM(aiMatrix4x4);

// recursive function used by Mesh::MultiFromFile() to process loaded assimp data
void ProcessNode(aiNode* node, const aiScene* scene, std::vector<std::pair<aiMesh*, aiNode*>>& meshes, std::vector<glm::mat4x4>& transformations, glm::mat4x4 nodeTransformation) {
    nodeTransformation = nodeTransformation * AssimpMatrixToGLM(node->mTransformation);
    
    // DebugLogInfo("Node ", node->mName.C_Str(), " has ", node->mNumChildren, "kids (but ",  node->mNumMeshes, " meshes)");
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(std::make_pair(mesh, node));
        transformations.push_back(nodeTransformation);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, meshes, transformations, nodeTransformation);
    }
}

// Appends a TextureCreateParams to params for the given aiMaterial.
void ProcessTextures(std::vector<TextureCreateParams>& params, aiTextureType aiType, Texture::TextureUsage engineType, aiMaterial* material, const aiScene* scene) {
    // don't really need a for loop because atm we don't support multiple textures of the same type on a material, but just in case
    for (unsigned int textureIndex = 0; textureIndex < material->GetTextureCount(aiType); textureIndex++) {
        // TODO: texture wrapping/other parameters, shaders???
        aiString texPath;
        material->GetTexture(aiType, textureIndex, &texPath);
        params.push_back(TextureCreateParams({std::string(texPath.C_Str())}, engineType));
        params.back().scene = scene;
    }

}

// returns index of bone the node corresponds to, or -1 if node is not a bone
int NodeIsBone(const std::vector<Bone>& bones, aiNode* node) {
    // DebugLogInfo("We've been asked if the node ", node->mName.C_Str(), " is a bone.");
    for (unsigned int i = 0; i < bones.size(); i++) {
        if (bones[i].name == node->mName.C_Str()) {
            return i;
        }
    }
    return -1;
}

// recursive function, sets the childrenBoneIndices of the bones vector and returns index of root bone
// will return -1 if no root bone was found
void BuildBoneHierarchy(std::vector<Bone>& bones, aiNode* node, int& rootIndex) {

    int boneIndex = NodeIsBone(bones, node);
    if (rootIndex == -1 && boneIndex != -1) { // then this node is a bone
        // DebugLogInfo("Node is bone ", node->mName.C_Str(), "  at ", boneIndex);
        // a bone is the root bone if its corresponding node has no parent, or its parent is not a bone
        if ((node->mParent == nullptr || NodeIsBone(bones, node->mParent) == -1)) { 
            // DebugLogInfo("NOde parent is NOT BONE, ", NodeIsBone(bones, node->mParent));
            rootIndex = boneIndex;
        }
    }

    // set bone children and call this function on child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {

        // if this node is a bone, then set bone children
        if (boneIndex != -1) {
            int childIndex = NodeIsBone(bones, node->mChildren[i]);
            if (childIndex != -1) { // make sure child is also a bone
                bones.at(boneIndex).childrenBoneIndices.push_back(childIndex);    
                bones.at(childIndex).parentIndex = boneIndex;
            }
        }
        
        // even if this node isn't a bone, its children might be, so call this function on them too
        // DebugLogInfo("trying child...");

        BuildBoneHierarchy(bones, node->mChildren[i], rootIndex);
        //if (foundRoot != -1) {
        //    Assert(rootBoneIndex == -1); // if this isn't the case, then we found two root bones somehow??
        //    rootBoneIndex = foundRoot;
        //}
    }

    //return rootBoneIndex;
}

glm::mat4x4 AssimpMatrixToGLM(aiMatrix4x4 mat) {
    // has to be transposed because assimp is row major, glm is column major
    return glm::transpose(glm::mat4x4(
        mat.a1, mat.a2, mat.a3, mat.a4,
        mat.b1, mat.b2, mat.b3, mat.b4,
        mat.c1, mat.c2, mat.c3, mat.c4,
        mat.d1, mat.d2, mat.d3, mat.d4
    ));
} 

glm::vec3 AssimpVecToGLM(aiVector3D v) {
    return glm::vec3 {v.x, v.y, v.z};
}

glm::quat AssimpQuatToGLM(aiQuaternion q) {
    return glm::quat(q.w, q.x, q.y, q.z);
}

// TODO: animation processing especially is probably hecka slow.
// TODO: i have my doubts on how well different vertex formats are handled
std::vector<Mesh::MeshRet> Mesh::MultiFromFile(const std::string& path, const MeshCreateParams& params) {
    //static Assimp::Importer importer;
    //const aiScene* scene = importer.ReadFileFromMemory(nullptr, 0, 0, nullptr);
    //const aiScene* scene = importer.ReadFile(path, aiProcess_OptimizeMeshes  | aiProcess_GlobalScale | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    //importer.SetPropertyBool(AI_CONFIG_FBX_CONVERT_TO_M, true);
    //aiSetImportPropertyInteger()
    const aiScene* scene = aiImportFile(path.c_str(), aiProcess_OptimizeMeshes | aiProcess_GlobalScale | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality | aiProcess_OptimizeGraph | aiProcess_PopulateArmatureData);
    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
        aiReleaseImport(scene);
        DebugLogError("Mesh::MultiFromFile() failed to load ", path, " because ", aiGetErrorString());
        abort();
    }
    
    
    // DebugLogInfo("Scene has ", scene->mNumMeshes, " root ", scene->mRootNode->mNumMeshes, " root kids ", scene->mRootNode->mNumChildren);

    std::vector<glm::mat4x4> assimpMeshTransformations;
    std::vector<std::pair<aiMesh*, aiNode*>> assimpMeshes;
    ProcessNode(scene->mRootNode, scene, assimpMeshes, assimpMeshTransformations, glm::identity<glm::mat4x4>());
    // for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    //     assimpMeshes.push_back(scene->mMeshes[i]);
    // }

    std::vector<MeshRet> returnValue;

    int i = 0;
    for (auto & [mesh, meshNode]: assimpMeshes) {
        glm::mat4x4 transform = assimpMeshTransformations[i++];
        // Assert(mesh->mNumUVComponents == 2);

        // bones needs to be multiple of 4 for mesh vertex format (so that meshpools can group up animated meshes efficiently)
        unsigned int roundedBoneCount = 0;
        if (mesh->HasBones()) {
            roundedBoneCount = 4;
            while (roundedBoneCount < mesh->mNumBones) {
                roundedBoneCount *= 4;
            }
        }

        
        MeshVertexFormat format = params.meshVertexFormat.has_value() ? params.meshVertexFormat.value() : MeshVertexFormat(
            {
                .position = VertexAttribute {.nFloats = 3, .instanced = false},
                .textureUV = mesh->HasTextureCoords(0) ? std::make_optional(VertexAttribute {.nFloats = 2, .instanced = false}) : std::nullopt,   
                .textureZ = VertexAttribute {.nFloats = 1, .instanced = true},
                .color = VertexAttribute {.nFloats = 4, .instanced = !mesh->HasVertexColors(0)}, 
                .modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
                .normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true},
                .normal = mesh->HasNormals() ? std::make_optional(VertexAttribute {.nFloats = 3, .instanced = false}) : std::nullopt,
                .tangent = mesh->HasTangentsAndBitangents() ? std::make_optional(VertexAttribute {.nFloats = 3, .instanced = false}) : std::nullopt,
                .arbitrary1 = roundedBoneCount != 0 ? std::optional(VertexAttribute {.nFloats = 4, .instanced = false, .integer = true}) : std::nullopt,
                .arbitrary2 = roundedBoneCount != 0 ? std::optional(VertexAttribute {.nFloats = 4, .instanced = false}) : std::nullopt
            },  
            roundedBoneCount != 0,
            roundedBoneCount
        );

        //format.primitiveType = GL_POINTS;

        // TODO: custom exceptions
        if ((format.attributes.position.has_value() && !format.attributes.position->instanced) && !mesh->HasPositions()) {
            throw std::runtime_error(std::string("The mesh \"") + mesh->mName.C_Str() + "\" in \"" + path + "\" lacks vertex positions. Why.");
        } 
        if ((format.attributes.textureUV.has_value() && !format.attributes.textureUV->instanced) && !mesh->HasTextureCoords(0)) {
            throw std::runtime_error(std::string("The mesh \"") + mesh->mName.C_Str() + "\" in \"" + path + "\" lacks UVs.");
        } 
        if ((format.attributes.color.has_value() && !format.attributes.color->instanced) && !mesh->HasVertexColors(0)) {
            throw std::runtime_error(std::string("The mesh \"") + mesh->mName.C_Str() + "\" in \"" + path + "\" lacks vertex colors (and you asked for them).");
        } 
        // assimp should have calculated these
        Assert(mesh->HasNormals());
        Assert(!format.attributes.textureUV.has_value() || mesh->HasTangentsAndBitangents()); // can't have tangent/bitangent if no UVs

        Assert(mesh->HasFaces());

        // MeshVertexFormat::FormatVertexAttributes attributes;
        // if (mesh->HasPositions()) {
        //     attributes.position = VertexAttribute(.nFloats = 3, .instanced = false);
        // }

        // DebugLogInfo("Bones = ", roundedBoneCount);

        // MeshVertexFormat format = MeshVertexFormat::Default(nBones, !mesh->HasVertexColors(0), true);

        std::vector<GLfloat> vertices;
        vertices.resize(mesh->mNumVertices * format.GetNonInstancedVertexSize()/sizeof(GLfloat));

        if (mesh->GetNumColorChannels() > 1) {
            DebugLogError("Warning: the mesh ", mesh->mName.C_Str(), " in ", path, " has ", mesh->GetNumColorChannels(), " color channels. Only the first one can be used! ");
          
        }

        if (mesh->GetNumUVChannels() > 1) { // TODO: mainly used for lightmaps (not applicable to us), but still sometimes used for other stuff. Assimp creates a bunch of UV channels (maybe?), one for each texture.
            DebugLogError("Warning: the mesh ", mesh->mName.C_Str(), " in ", path, " has ", mesh->GetNumUVChannels(), " UV channels. Only the first one can be used! ");
            // names were remarkably unhelpful
            // for (unsigned int nameI = 0; nameI < mesh->GetNumUVChannels(); nameI++) {
            //     DebugLogInfo("One is named ", mesh->GetTextureCoordsName(nameI)->C_Str());
            // }
        }
        // Assert(mesh->GetNumUVChannels() > 0);

        // DebugLogInfo("vert count = ", mesh->mNumVertices, " noninst.size = ", format.GetNonInstancedVertexSize(), " vector len = ", vertices.size(), " pos offset = ", format.attributes.position->offset);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            
            if (format.attributes.position.has_value() && !format.attributes.position->instanced) {
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/sizeof(GLfloat)] = mesh->mVertices[i].x;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/sizeof(GLfloat) + 1] = mesh->mVertices[i].y;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/sizeof(GLfloat) + 2] = mesh->mVertices[i].z;
            }
            // DebugLogInfo("Pushed vertex ", mesh->mVertices[i].x, " ", mesh->mVertices[i].y, " ", mesh->mVertices[i].z, " at ", i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/4);

            if (format.attributes.normal.has_value() && !format.attributes.normal->instanced) {
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.normal->offset/sizeof(GLfloat)] = mesh->mNormals[i].x;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.normal->offset/sizeof(GLfloat) + 1] = mesh->mNormals[i].y;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.normal->offset/sizeof(GLfloat) + 2] = mesh->mNormals[i].z;
            }

            if (format.attributes.tangent.has_value() && !format.attributes.tangent->instanced) {
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.tangent->offset/sizeof(GLfloat)] = mesh->mTangents[i].x;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.tangent->offset/sizeof(GLfloat) + 1] = mesh->mTangents[i].x;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.tangent->offset/sizeof(GLfloat) + 2] = mesh->mTangents[i].x;
            }

            if (format.attributes.textureUV.has_value() && !format.attributes.textureUV->instanced) {
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.textureUV->offset/sizeof(GLfloat)] = mesh->mTextureCoords[0][i].x;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.textureUV->offset/sizeof(GLfloat) + 1] = mesh->mTextureCoords[0][i].y;
            }
            
            if (format.attributes.color.has_value() && !format.attributes.color.value().instanced) {
                // DebugLogInfo("adding color.");
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat)] = mesh->mColors[0][i].r;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat) + 1] = mesh->mColors[0][i].g;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat) + 2] = mesh->mColors[0][i].b;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat) + 3] = mesh->mColors[0][i].a;
            }

            // bone ids/weights done for realsies later bc assimp is weird; this just supplies -1 as default for boneIds
            if (format.attributes.arbitrary1.has_value() && !format.attributes.arbitrary1.value().instanced) {
                // DebugLogInfo("adding color.");
                vertices[i * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + format.attributes.arbitrary1->offset / sizeof(GLint)] = reinterpret_cast<const float&>((const int&)(-1));
                vertices[i * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + format.attributes.arbitrary1->offset / sizeof(GLint) + 1] = reinterpret_cast<const float&>((const int&)(-1));
                vertices[i * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + format.attributes.arbitrary1->offset / sizeof(GLint) + 2] = reinterpret_cast<const float&>((const int&)(-1));
                vertices[i * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + format.attributes.arbitrary1->offset / sizeof(GLint) + 3] = reinterpret_cast<const float&>((const int&)(-1));
            }

        }

        std::vector<GLuint> indices;
        indices.resize(mesh->mNumFaces * 3);
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices[i * 3 + j] = face.mIndices[j];
            }
        }  


        std::shared_ptr<Material> matPtr = nullptr;
        float textureZ = -1.0;
        if (mesh->mMaterialIndex >= 0) { // if the mesh has a material
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            // todo: multiple texture support?
            Assert(material->GetTextureCount(aiTextureType_DIFFUSE) <= 1);
            Assert(material->GetTextureCount(aiTextureType_SPECULAR) <= 1);
            Assert(material->GetTextureCount(aiTextureType_DISPLACEMENT) <= 1);
            Assert(material->GetTextureCount(aiTextureType_NORMALS) <= 1);

            std::vector<TextureCreateParams> texParams;
            ProcessTextures(texParams, aiTextureType_DIFFUSE, Texture::TextureUsage::ColorMap, material, scene);
            ProcessTextures(texParams, aiTextureType_SPECULAR, Texture::TextureUsage::SpecularMap, material, scene);
            ProcessTextures(texParams, aiTextureType_DISPLACEMENT, Texture::TextureUsage::DisplacementMap, material, scene);
            ProcessTextures(texParams, aiTextureType_NORMALS, Texture::TextureUsage::NormalMap, material, scene);
            
            if (texParams.size() == 0) { 
                // then it lied and there is no material
            }
            else {
                try {
                    auto mat = Material::Copy(GraphicsEngine::Get().defaultMaterial);
                    auto [textures, layer] = TextureCollection::FindCollection(MaterialCreateParams{ .textureParams = texParams, .type = Texture::TextureType::Texture2D });
                    mat->textures = textures;
                    matPtr = mat;
                    textureZ = layer;
                }
                catch (std::runtime_error& error) {
                    DebugLogError("In the loading of the material \"", material->GetName().C_Str(), "\" for the mesh \"", mesh->mName.C_Str(), "\" from the file \"", path, "\", the following exception was thrown:\n\t", error.what(), "\n\tUsing fallback texture.");
                    matPtr = GraphicsEngine::Get().errorMaterial;
                    textureZ = GraphicsEngine::Get().errorMaterialTextureZ;
                }
            }
        }

        std::optional<std::vector<Animation>> animations;
        std::optional<std::vector<Bone>> bones;
        
        //DebugLogInfo("There are, ", mesh->mNumBones);

        int rootBoneIndex = -1;


        if (mesh->mNumBones > 0) {
            bones.emplace();


            std::unordered_map<aiNode*, int> nodesToBoneIndices;
            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
                nodesToBoneIndices[mesh->mBones[boneIndex]->mNode] = boneIndex;
            }

            std::vector<unsigned int> numBonesOnEachVertex;
            numBonesOnEachVertex.resize(vertices.size(), 0);
            Assert(format.attributes.arbitrary1.has_value() && !format.attributes.arbitrary1.value().instanced); // arb1 = bone ids
            Assert(format.attributes.arbitrary2.has_value() && !format.attributes.arbitrary2.value().instanced); // arb2 = bone weights
            
            std::unordered_map<std::string, unsigned int> boneNamesToIds;
            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
                aiBone* bone = mesh->mBones[boneIndex];
                //DebugLogInfo("Bone named ", bone->mName.C_Str(), " has ", bone->mNumWeights);

                bones->emplace_back(Bone{
                    .name = bone->mName.C_Str(),
                    .id = boneIndex,
                    .childrenBoneIndices = {}, // done later
                    .parentIndex = -1, // done later
                    .inverseBindTransform = AssimpMatrixToGLM(bone->mOffsetMatrix),                    
                    //.baseBonePosition = done later 
                });


                boneNamesToIds[bone->mName.C_Str()] = boneIndex;

                // set bone weight/ids for vertex attributes
                for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
                    aiVertexWeight weight = bone->mWeights[weightIndex];
                    
                    // we only support 4 bones affecting a single vertex, so we need to check.
                    // also, apparently faces generally need more than 4 bones
                    
                    unsigned int numBonesAffectingThisVertex = numBonesOnEachVertex[weight.mVertexId];
                    unsigned int boneIdIndex = weight.mVertexId * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + format.attributes.arbitrary1->offset / sizeof(GLint);
                    unsigned int boneWeightIndex = weight.mVertexId * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.arbitrary2->offset/sizeof(GLfloat);
                    if (numBonesAffectingThisVertex < 4) {
                        
                        vertices[boneIdIndex + numBonesAffectingThisVertex] = reinterpret_cast<const float&>((const int&)(boneIndex));
                        vertices[boneWeightIndex + numBonesAffectingThisVertex] = weight.mWeight; 
                        numBonesOnEachVertex[weight.mVertexId] += 1;
                        //DebugLogInfo("Wrote weight ", weight.mWeight, " and id ", boneIndex, " to ", boneIdIndex + numBonesAffectingThisVertex);
                    }
                    else {  // if there's already 4, we'll see if there's one with less weight than this one, and if so replace that one
                        // TODO: this isn't great when more than 4 bones because doesn't make sure it adds up to 1
                        for (unsigned int vertexWeightIndex = 0; vertexWeightIndex < 4; vertexWeightIndex++) {
                            if (vertices[boneWeightIndex + vertexWeightIndex] < weight.mWeight) {
                                vertices[boneIdIndex + vertexWeightIndex] = reinterpret_cast<const float&>((const int&)(boneIndex));
                                vertices[boneWeightIndex + vertexWeightIndex] = weight.mWeight;
                            }
                        }
                    }

                }
            }

            auto findBoneParent = [&](aiBone* bone, glm::mat4x4& nodeTransform) -> int {
                auto currentNode = bone->mNode;

                nodeTransform = AssimpMatrixToGLM(currentNode->mTransformation) * nodeTransform;

                while (currentNode && currentNode != meshNode) {

                    

                    if (nodesToBoneIndices.contains(currentNode->mParent)) {
                        
                        return nodesToBoneIndices[currentNode->mParent];
                    }
                    else {
                        currentNode = currentNode->mParent;
                    }
                }

                return -1;
            };

            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
                aiBone* bone = mesh->mBones[boneIndex];
                //DebugLogInfo("Bone named ", bone->mName.C_Str(), " has ", bone->mNumWeights);

                // Find bone children/parent and the bone's transform in the bind pose
                glm::mat4x4 bindPoseTransform = glm::identity<glm::mat4x4>();

                int parent = findBoneParent(bone, bindPoseTransform);
                bones.value()[boneIndex].baseBonePosition = bindPoseTransform;
                if (parent == -1) {
                    Assert(rootBoneIndex == -1);
                    rootBoneIndex = boneIndex;
                }
                else {
                    bones.value()[boneIndex].parentIndex = parent;
                    bones.value()[parent].childrenBoneIndices.push_back(boneIndex);
                }
            }

            animations.emplace();
            for (unsigned int animIndex = 0; animIndex < scene->mNumAnimations; animIndex++) {
                aiAnimation* anim = scene->mAnimations[animIndex];

                // TODO: warn (or support) the other anim channel types here

                // we need to determine if this animation actually affects this mesh (since assimp does them on a whole-scene basis), by seeing if it affects any of its bones.
                unsigned int numAffectedBones = 0;
                for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; channelIndex++) {
                    aiNodeAnim* channel = anim->mChannels[channelIndex];
                    // DebugLogInfo("Channel named ", channel->mNodeName.C_Str());
                    // channel.

                    if (boneNamesToIds.count(channel->mNodeName.C_Str())) { // then we care about this animation for this mesh. Add ALL the stuff to it.
                        numAffectedBones++;
                    }
                }

                if (numAffectedBones > 0) {
                    if (anim->mTicksPerSecond == 0) { // some files don't say the tickrate, in which case assimp gives us 0 which won't work
                        DebugLogError("Warning: loaded animation has unknown TPS. Assuming TPS of 30.");
                        anim->mTicksPerSecond = 30;
                    } 
                    float animLengthInSeconds = anim->mDuration/anim->mTicksPerSecond;
                    float secondsPerTick = 1.0/anim->mTicksPerSecond;

                    std::vector<BoneAnimation> boneAnims;
                    
                    // each ASSIMP animation channel affects one node (and in our case nodes are bones)
                    for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; channelIndex++) {
                        aiNodeAnim* channel = anim->mChannels[channelIndex];
                        Assert(channel->mNumPositionKeys == channel->mNumRotationKeys);
                        Assert(channel->mNumPositionKeys == channel->mNumScalingKeys);

                        // TODO: we should have some way to handle animations affecting nodes that aren't bones
                        if (!boneNamesToIds.contains(channel->mNodeName.C_Str())) 
                            continue;  // since animations are done on a whole-scene basis, the animation will likely affect stuff we don't care about.
                        
                        unsigned int boneId = boneNamesToIds.at(channel->mNodeName.C_Str());

                        std::vector<BoneKeyframe> keyframes;

                        //DebugLogInfo("Ok so channel ", channel, " for bone ", channel->mNodeName.C_Str(), " has ", channel->mNumPositionKeys, " vs ", keyframes.size());
                        for (unsigned int keyframeI = 0; keyframeI < channel->mNumPositionKeys; keyframeI++) {
                            // DebugLogInfo("its ", channel->mNumPositionKeys, " ", channel->mNumRotationKeys);
                            // DebugLogInfo("Pos keyframe at ", channel->mPositionKeys[keyframeIndex].mTime);
                            Assert(channel->mPositionKeys[keyframeI].mTime == channel->mRotationKeys[keyframeI].mTime);
                            Assert(channel->mPositionKeys[keyframeI].mTime == channel->mScalingKeys[keyframeI].mTime);
                            // Assert(channel->mPositionKeys[keyframeIndex].mTime == keyframeIndex);
                            keyframes.push_back(BoneKeyframe{
                                .translation = AssimpVecToGLM(channel->mPositionKeys[keyframeI].mValue),
                                .scale = AssimpVecToGLM(channel->mScalingKeys[keyframeI].mValue),
                                .rotation = AssimpQuatToGLM(channel->mRotationKeys[keyframeI].mValue),
                                .timestamp = (float)channel->mPositionKeys[keyframeI].mTime * secondsPerTick
                            });

                        }

                        boneAnims.push_back(BoneAnimation{
                            .keyframes = keyframes,
                            .boneIndex = boneId
                        });
                    }

                    animations->push_back(Animation {
                        .name = anim->mName.C_Str(),
                        .duration = animLengthInSeconds,
                        .priority = 0,
                        .boneAnimations = boneAnims
                    });

                }
                
            }
        }



        if (mesh->mNumAnimMeshes > 0) { // vertex based animation directly changes vertex positions/other attributes instead of doing it through bones. TODO: might not be too hard to support?
            DebugLogError("Warning: the mesh ", mesh->mName.C_Str(), " in ", path, " has vertex based animation, which is not supported by AG3. Sorry!");
        }

        // }

        //int rootBoneIndex = 0;
        //if (bones.has_value()) {
        //    rootBoneIndex = -1;
        //    BuildBoneHierarchy(bones.value(), scene->mRootNode, rootBoneIndex); // setup bone hierarchy and find the root bone
        //}
        //Assert(rootBoneIndex != -1); 
        //if (bones.has_value()) {
        //    //DebugLogInfo("The root of ", mesh->mName.C_Str(), " is ", rootBoneIndex, " aka ", bones->at(rootBoneIndex).name);
        //}
        
        

        MeshCreateParams makeMeshParams = params;
        makeMeshParams.meshVertexFormat.emplace(format);

        if (bones.has_value()) Assert(rootBoneIndex != -1);

        unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
        auto meshPtr = std::shared_ptr<Mesh>(new Mesh(MeshConstructorArgs{
            .verts = vertices,
            .indies = indices,
            .params = makeMeshParams,
            .dynamic = false,
            .fromText = false,
            .bones = (bones.has_value() && bones->size() > 0) ? bones : std::nullopt,
            .anims = (animations.has_value() && animations->size() > 0) ? animations : std::nullopt,
            .rootBoneIndex = bones.has_value() ? (unsigned)rootBoneIndex : 0
        }));
        
        MeshGlobals::Get().LOADED_MESHES[meshId] = meshPtr;

        // split the mesh's transformation matrix into position, scale, rotation, etc.
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(transform, scale, rotation, translation, skew, perspective);
        translation = glm::vec3(transform * glm::vec4(0, 0, 0, 0));
        meshPtr->originalSize *= scale;
        if (glm::epsilonNotEqual(glm::length(skew), 0.0f, 0.0001f)) {
            DebugLogError("Warning: mesh ", mesh->mName.C_Str(), " at ", path, " has skew of ", skew, ". Skew is not supported.");
        }
        if (glm::epsilonNotEqual(glm::length(perspective),1.0f, 0.0001f)) {
            DebugLogError("Warning: mesh ", mesh->mName.C_Str(), " at ", path, " has perspective transformation of ", perspective, ", which will be ignored. Something is very wrong with your file.");
        }
            
        

        //DebugLogInfo("ADDING ", meshPtr);
        returnValue.push_back(MeshRet{.mesh = meshPtr, .material = matPtr, .materialZ = textureZ, .posOffset = translation, .rotOffset = rotation});
    }

    aiReleaseImport(scene);

    return returnValue;
}