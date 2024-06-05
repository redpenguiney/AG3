#include "mesh.hpp"
#include "material.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// recursive function used by Mesh::MultiFromFile() to process loaded assimp data
void processNode(aiNode* node, const aiScene* scene, std::vector<aiMesh*>& meshes) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(mesh);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes);
    }
}

std::vector<std::tuple<std::shared_ptr<Mesh>, std::shared_ptr<Material>, float, glm::vec3>> Mesh::MultiFromFile(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_OptimizeMeshes | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
        DebugLogError("Mesh::MultiFromFile() failed to load ", path, " because ", importer.GetErrorString());
        abort();
    }
    
    // DebugLogInfo("Scene has ", scene->mNumMeshes, " root ", scene->mRootNode->mNumMeshes, " root kids ", scene->mRootNode->mNumChildren);

    std::vector<aiMesh*> assimpMeshes;
    processNode(scene->mRootNode, scene, assimpMeshes);
    // ok since the root node makes me segfault (??? TODO ???) we just do this intead
    // for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    //     assimpMeshes.push_back(scene->mMeshes[i]);
    // }

    std::vector<std::tuple<std::shared_ptr<Mesh>, std::shared_ptr<Material>, float, glm::vec3>> returnValue;

    for (auto & mesh: assimpMeshes) {
        // assert(mesh->mNumUVComponents == 2);

        // MeshVertexFormat format {
        //     .attributes = {
        //         .position = VertexAttribute {.offset = 0, .nFloats = 3, .instanced = false},
        //         .textureUV = VertexAttribute {.offset = sizeof(glm::vec3), .nFloats = 2, .instanced = false},   
        //         .textureZ = VertexAttribute {.offset = (unsigned short)sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + sizeof(glm::vec4), .nFloats = 1, .instanced = true},
        //         .color = VertexAttribute {.offset = (unsigned short)(sizeof(glm::mat4x4) + sizeof(glm::mat3x3)), .nFloats = 4, .instanced = true}, 
        //         .modelMatrix = VertexAttribute {.offset = 0, .nFloats = 16, .instanced = true},
        //         .normalMatrix = VertexAttribute {.offset = sizeof(glm::mat4x4), .nFloats = 9, .instanced = true},
        //         .normal = VertexAttribute {.offset = sizeof(glm::vec3) + sizeof(glm::vec2), .nFloats = 3, .instanced = false},   
        //         .tangent = VertexAttribute {.offset = 2 * sizeof(glm::vec3) + sizeof(glm::vec2), .nFloats = 3, .instanced = false},
        //     }
            
        // };

        MeshVertexFormat format = MeshVertexFormat::Default(!mesh->HasVertexColors(0), true);

        std::vector<GLfloat> vertices;
        vertices.resize(mesh->mNumVertices * format.GetNonInstancedVertexSize());

        assert(mesh->HasPositions());
        assert(mesh->HasFaces());
        assert(mesh->HasNormals());
        assert(mesh->HasTangentsAndBitangents());

        assert(mesh->GetNumColorChannels() <= 1);
        assert(mesh->GetNumUVChannels() <= 1);

        // DebugLogInfo("vert count = ", mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/sizeof(GLfloat)] = mesh->mVertices[i].x;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/sizeof(GLfloat) + 1] = mesh->mVertices[i].y;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/sizeof(GLfloat) + 2] = mesh->mVertices[i].z;
            
            // DebugLogInfo("Pushed vertex ", mesh->mVertices[i].x, " ", mesh->mVertices[i].y, " ", mesh->mVertices[i].z, " at ", i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.position->offset/4);

            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.normal->offset/sizeof(GLfloat)] = mesh->mNormals[i].x;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.normal->offset/sizeof(GLfloat) + 1] = mesh->mNormals[i].y;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.normal->offset/sizeof(GLfloat) + 2] = mesh->mNormals[i].z;
            
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.tangent->offset/sizeof(GLfloat)] = mesh->mTangents[i].x;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.tangent->offset/sizeof(GLfloat) + 1] = mesh->mTangents[i].x;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.tangent->offset/sizeof(GLfloat) + 2] = mesh->mTangents[i].x;

            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.textureUV->offset/sizeof(GLfloat)] = mesh->mTextureCoords[0][i].x;
            vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.textureUV->offset/sizeof(GLfloat) + 1] = mesh->mTextureCoords[0][i].y;
            
            if (!format.attributes.color.value().instanced) {
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat)] = mesh->mColors[0][i].r;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat) + 1] = mesh->mColors[0][i].g;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat) + 2] = mesh->mColors[0][i].b;
                vertices[i * format.GetNonInstancedVertexSize()/sizeof(GLfloat) + format.attributes.color->offset/sizeof(GLfloat) + 3] = mesh->mColors[0][i].a;
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
            assert(material->GetTextureCount(aiTextureType_DIFFUSE) == 1);
            assert(material->GetTextureCount(aiTextureType_SPECULAR) <= 1);
            assert(material->GetTextureCount(aiTextureType_DISPLACEMENT) <= 1);
            assert(material->GetTextureCount(aiTextureType_NORMALS) <= 1);

            aiString colorPath, specularPath, displacementPath, normalsPath;
            // if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                material->GetTexture(aiTextureType_DIFFUSE, 0, &colorPath);
            // }
            
            material->GetTexture(aiTextureType_SPECULAR, 0, &specularPath);
            material->GetTexture(aiTextureType_DISPLACEMENT, 0, &displacementPath);
            material->GetTexture(aiTextureType_NORMALS, 0, &normalsPath);

            auto pair = Material::New({
                TextureCreateParams({colorPath.C_Str()}, Texture::TextureUsage::ColorMap)
            }, Texture::TextureType::Texture2D);
            matPtr = pair.second;
            textureZ = pair.first;
        }

        unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
        auto meshPtr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, MeshCreateParams {.meshVertexFormat = format, .expectedCount = 64, .normalizeSize = true, .dynamic = false }));
        MeshGlobals::Get().LOADED_MESHES[meshId] = meshPtr;
            
        returnValue.push_back(std::make_tuple(meshPtr, matPtr, textureZ, glm::vec3(0, 0, 0)));
    }

    return returnValue;
}