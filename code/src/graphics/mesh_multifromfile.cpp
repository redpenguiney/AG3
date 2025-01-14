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
void ProcessNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& meshes, std::vector<glm::mat4x4>& transformations, glm::mat4x4 nodeTransformation) {
    nodeTransformation = nodeTransformation * AssimpMatrixToGLM(node->mTransformation);
    
    // DebugLogInfo("Node ", node->mName.C_Str(), " has ", node->mNumChildren, "kids (but ",  node->mNumMeshes, " meshes)");
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(mesh);
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
int BuildBoneHierarchy(std::vector<Bone>& bones, aiNode* node, bool rootAlreadyFound = false) {

    int rootBoneIndex = -1;

    int boneIndex = NodeIsBone(bones, node);
    if (!rootAlreadyFound && boneIndex != -1) { // then this node is a bone
        // DebugLogInfo("Node is bone ", node->mName.C_Str(), "  at ", boneIndex);
        // a bone is the root bone if its corresponding node has no parent, or its parent is not a bone
        if ((node->mParent == nullptr || NodeIsBone(bones, node->mParent) == -1)) { 
            // DebugLogInfo("NOde parent is NOT BONE, ", NodeIsBone(bones, node->mParent));
            rootBoneIndex = boneIndex;
        }
    }

    // set bone children and call this function on child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {

        // if this node is a bone, then set bone children
        if (boneIndex != -1) {
            if (int childIndex = NodeIsBone(bones, node->mChildren[i])) { // make sure child is also a bone
                bones.at(boneIndex).childrenBoneIndices.push_back(childIndex);    
            }
        }
        
        // even if this node isn't a bone, its children might be, so call this function on them too
        // DebugLogInfo("trying child...");
        int foundRoot = BuildBoneHierarchy(bones, node->mChildren[i], rootBoneIndex != -1 ? true : rootAlreadyFound);
        if (foundRoot != -1) {
            Assert(rootBoneIndex == -1); // if this isn't the case, then we found two root bones somehow??
            rootBoneIndex = foundRoot;
        }
    }

    return rootBoneIndex;
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
    const aiScene* scene = aiImportFile(path.c_str(), aiProcess_OptimizeMeshes | aiProcess_GlobalScale | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
        aiReleaseImport(scene);
        DebugLogError("Mesh::MultiFromFile() failed to load ", path, " because ", aiGetErrorString());
        abort();
    }
    
    
    // DebugLogInfo("Scene has ", scene->mNumMeshes, " root ", scene->mRootNode->mNumMeshes, " root kids ", scene->mRootNode->mNumChildren);

    std::vector<glm::mat4x4> assimpMeshTransformations;
    std::vector<aiMesh*> assimpMeshes;
    ProcessNode(scene->mRootNode, scene, assimpMeshes, assimpMeshTransformations, glm::identity<glm::mat4x4>());
    // for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    //     assimpMeshes.push_back(scene->mMeshes[i]);
    // }

    std::vector<MeshRet> returnValue;

    int i = 0;
    for (auto & mesh: assimpMeshes) {
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

            // bone ids/weights done below bc assimp is weird
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
                    auto pair = Material::New(MaterialCreateParams{ .textureParams = texParams, .type = Texture::TextureType::Texture2D });
                    matPtr = pair.second;
                    textureZ = pair.first;
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
        if (mesh->mNumBones > 0) {
            bones.emplace();

            std::vector<unsigned int> numBonesOnEachVertex;
            numBonesOnEachVertex.resize(vertices.size(), 0);
            Assert(format.attributes.arbitrary1.has_value() && !format.attributes.arbitrary1.value().instanced); // arb1 = bone ids
            Assert(format.attributes.arbitrary2.has_value() && !format.attributes.arbitrary2.value().instanced); // arb2 = bone weights
            
            std::unordered_map<std::string, unsigned int> boneNamesToIds;
            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
                aiBone* bone = mesh->mBones[boneIndex];
                DebugLogInfo("Bone named ", bone->mName.C_Str(), " has ", bone->mNumWeights);
                bones->emplace_back(Bone {
                    .name = bone->mName.C_Str(),
                    .id = boneIndex,
                    .localBoneTransform = AssimpMatrixToGLM(bone->mOffsetMatrix),
                });

                boneNamesToIds[bone->mName.C_Str()] = boneIndex;

                // set bone weight/ids for vertex attributes
                for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
                    aiVertexWeight weight = bone->mWeights[weightIndex];
                    
                    // we only support 4 bones affecting a single vertex, so we need to check.
                    // also, apparently faces generally need more than 4 bones
                    
                    unsigned int numBonesAffectingThisVertex = numBonesOnEachVertex[weight.mVertexId];
                    unsigned int boneIdIndex = indices[weight.mVertexId] * format.GetNonInstancedVertexSize() / sizeof(GLfloat) + format.attributes.arbitrary1->offset / sizeof(GLint);
                    unsigned int boneWeightIndex = indices[weight.mVertexId] * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.arbitrary2->offset/sizeof(GLfloat);
                    if (numBonesAffectingThisVertex < 4) {
                        numBonesOnEachVertex[weight.mVertexId] += 1;
                        vertices[boneIdIndex + numBonesAffectingThisVertex] = reinterpret_cast<const float&>((const int&)(boneIndex));
                        vertices[boneWeightIndex + numBonesAffectingThisVertex] = weight.mWeight; 
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

                    std::vector<AnimationKeyframe> keyframes;
                    // our keyframe class is all the bone positions at one timestamp, while assimp's is all the timestamps for one bone.
                    keyframes.resize(anim->mDuration + 1); // mDuration is num ticks, there's one keyframe per tick plus one for the end keyframe.
                    {
                        
                        // DebugLogInfo("Len = ", animLengthInSeconds, " tps = ", anim->mTicksPerSecond, " duration in ticks =  ", anim->mDuration);
                        for (unsigned int keyFrameIndex = 0; keyFrameIndex < anim->mDuration + 1; keyFrameIndex++) {
                            auto & keyframe = keyframes.at(keyFrameIndex);
                            keyframe.timestamp = keyFrameIndex * secondsPerTick;
                            // keyframe.boneKeyframes.resize(numAffectedBones);
                        }   
                    }
                    

                    for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; channelIndex++) {
                        aiNodeAnim* channel = anim->mChannels[channelIndex];

                        Assert(channel->mNumPositionKeys == channel->mNumRotationKeys);
                        Assert(channel->mNumPositionKeys == channel->mNumScalingKeys);
                        if (channel->mNumPositionKeys == 1) {
                            continue;
                        }
                        unsigned int boneId = boneNamesToIds.at(channel->mNodeName.C_Str());
                        unsigned int keyframeIndex = 0;

                        //DebugLogInfo("Ok so channel ", channel, " for bone ", channel->mNodeName.C_Str(), " has ", channel->mNumPositionKeys, " vs ", keyframes.size());
                        for (auto & keyframe: keyframes) {
                            // DebugLogInfo("its ", channel->mNumPositionKeys, " ", channel->mNumRotationKeys);
                            Assert(channel->mNumPositionKeys > keyframeIndex);
                            // DebugLogInfo("Pos keyframe at ", channel->mPositionKeys[keyframeIndex].mTime);
                            Assert(channel->mPositionKeys[keyframeIndex].mTime == channel->mRotationKeys[keyframeIndex].mTime);
                            Assert(channel->mPositionKeys[keyframeIndex].mTime == channel->mScalingKeys[keyframeIndex].mTime);
                            // Assert(channel->mPositionKeys[keyframeIndex].mTime == keyframeIndex);
                            keyframe.boneKeyframes.push_back(BoneKeyframe {
                                .boneIndex = boneId,
                                .translation = AssimpVecToGLM(channel->mPositionKeys[keyframeIndex].mValue),
                                .scale = AssimpVecToGLM(channel->mScalingKeys[keyframeIndex].mValue),
                                .rotation = AssimpQuatToGLM(channel->mRotationKeys[keyframeIndex].mValue)
                            });
                            // DebugLogInfo("Keyframe has ", glm::to_string(keyframe.boneKeyframes.back().rotation));
                            keyframeIndex++;
                        }

                    }

                    animations->push_back(Animation {
                        .name = anim->mName.C_Str(),
                        .duration = animLengthInSeconds,
                        .priority = 0,
                        .keyframes = keyframes
                    });

                }
                
            }
        }



        if (mesh->mNumAnimMeshes > 0) { // vertex based animation directly changes vertex positions/other attributes instead of doing it through bones. TODO: might not be too hard to support?
            DebugLogError("Warning: the mesh ", mesh->mName.C_Str(), " in ", path, " has vertex based animation, which is not supported by AG3. Sorry!");
        }

        // for (unsigned int animIndex = 0; animIndex < scene->mNumAnimations; animIndex++) {
        //     aiAnimation* aiAnim = scene->mAnimations[animIndex];
        //     // animations are done on a whole-scene basis, but we just want the part of the animation for this mesh
        //     for (unsigned int channelIndex = 0; channelIndex < aiAnim->mNumChannels; channelIndex++) {
        //         aiNodeAnim* meshAnim = aiAnim->mChannels[channelIndex];
        //         if (meshAnim) {
    
        //         }
        //         meshAnim.
        //     }
            
        // }

        int rootBoneIndex = bones.has_value() ? BuildBoneHierarchy(bones.value(), scene->mRootNode) : 0; // setup bone hierarchy and find the root bone
        Assert(rootBoneIndex != -1); 
        if (bones.has_value()) {
            //DebugLogInfo("The root of ", mesh->mName.C_Str(), " is ", rootBoneIndex, " aka ", bones->at(rootBoneIndex).name);
        }
        

        MeshCreateParams makeMeshParams = params;
        makeMeshParams.meshVertexFormat.emplace(format);

        unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
        auto meshPtr = std::shared_ptr<Mesh>(new Mesh(
            vertices, 
            indices, 
            makeMeshParams,
            false,
            false, 
            (bones.has_value() && bones->size() > 0) ? bones: std::nullopt, 
            (animations.has_value() && animations->size() > 0) ? animations : std::nullopt,
            rootBoneIndex
        ));
        
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