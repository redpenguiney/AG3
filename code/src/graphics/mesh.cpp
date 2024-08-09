#include "mesh.hpp"
#include "texture.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "glm/gtc/type_ptr.hpp"
#include "gengine.hpp"
#include <list>
#include "../debug/log.hpp"
#include "gameobjects/render_component.hpp"


// TODO: this file takes too long to compile



std::shared_ptr<Mesh> Mesh::New(const MeshProvider& provider, bool dynamic) {
    // create/return actual mesh object
	unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)

	auto realParams = provider.meshParams;
	if (realParams.meshVertexFormat.has_value() == false) { realParams.meshVertexFormat.emplace(MeshVertexFormat::Default()); }
    auto product = provider.GetMesh();
    auto mesh = std::shared_ptr<Mesh>(new Mesh(product.first, product.second, realParams, dynamic));
	MeshGlobals::Get().LOADED_MESHES[meshId] = mesh;
	return mesh;
}

MeshGlobals& MeshGlobals::Get() {
    #ifdef IS_MODULE
    Assert(_MESH_GLOBALS != nullptr);
    return *_MESH_GLOBALS;
    #else
    static MeshGlobals globals;
    return globals;
    #endif
}

MeshGlobals::MeshGlobals(): MESHES_TO_PHYS_MESHES() {}
MeshGlobals::~MeshGlobals() {}

// helper thing for Mesh::FromFile() and TextMeshFromText() that will expand the vector so that it contains index if needed, then return vector.at(index)
template<typename T>
typename std::vector<T>::reference vectorAtExpanding(unsigned int index, std::vector<T>& vector) {
    if (vector.size() <= index) {
        vector.resize(index + 1, 6.66); // 6.66 is chosen so that uninitialized values are easy to see when debugging
    }
    return vector.at(index);
}

bool Mesh::IsValidForGameObject(unsigned int meshId) {
    return MeshGlobals::Get().LOADED_MESHES.count(meshId) && MeshGlobals::Get().LOADED_MESHES.at(meshId)->instancedVertexSize > 0;
}

std::shared_ptr<Mesh> Mesh::Square() {
    const static std::vector<GLfloat> squareVerts = {
        -0.5, -0.5, 0.0,   0.0, 0.0, 
         0.5, -0.5, 0.0,   1.0, 0.0,
         0.5,  0.5, 0.0,   1.0, 1.0,
        -0.5,  0.5, 0.0,   0.0, 1.0,
    };
    const static std::vector<GLuint> squareIndices = { 0, 1, 2, 0, 2, 3 };

    // IF IT DOESN"T WORK LOOK HERE, i think this static variable is okay
    static std::shared_ptr<Mesh> m = Mesh::New(RawMeshProvider(squareVerts, squareIndices, MeshCreateParams {.meshVertexFormat = MeshVertexFormat::DefaultGui(), .opacity = 1.0, .expectedCount = 1, .normalizeSize = false}), false);
    return m;
}



unsigned int MeshVertexFormat::GetInstancedVertexSize() const {
    unsigned int size = 0;
    for (auto & atr: vertexAttributes) {
        if (atr.has_value() && atr->instanced) {
            size += atr->nFloats * sizeof(GLfloat);
        }
    }
    return size;
}

unsigned int MeshVertexFormat::GetNonInstancedVertexSize() const {
    unsigned int size = 0;
    for (auto & atr: vertexAttributes) {
        if (atr.has_value() && !atr->instanced) {
            size += atr->nFloats * sizeof(GLfloat);
        }
    }
    return size;
}

MeshVertexFormat::MeshVertexFormat(const MeshVertexFormat& other): supportsAnimation(other.supportsAnimation), maxBones(other.maxBones), attributes(other.attributes) {
    // we have to calculate attribute offsets
    unsigned int noninstancedOffset = 0, instancedOffset = 0;
    for (auto & attrib: vertexAttributes) {
        if (!attrib.has_value()) { continue; }
        if (attrib->instanced) {
            attrib->offset = instancedOffset;
            instancedOffset += attrib->nFloats * sizeof(GLfloat);
        }
        else {
            attrib->offset = noninstancedOffset;
            noninstancedOffset += attrib->nFloats * sizeof(GLfloat);
        }
    }
}

MeshVertexFormat::MeshVertexFormat(const MeshVertexFormat::FormatVertexAttributes& attrs, bool anims, unsigned int nBones): supportsAnimation(anims),  maxBones(nBones), attributes(attrs) {
    

    if (anims) {
        Assert(nBones == 4 || nBones == 16 || nBones == 64 || nBones == 256 || nBones == 1024 || nBones == 4096 || nBones == 16384);
    }

    // we have to calculate attribute offsets
    unsigned int noninstancedOffset = 0, instancedOffset = 0;
    for (auto & attrib: vertexAttributes) {
        if (!attrib.has_value()) { continue; }
        if (attrib->instanced) {
            attrib->offset = instancedOffset;
            instancedOffset += attrib->nFloats * sizeof(GLfloat);
        }
        else {
            attrib->offset = noninstancedOffset;
            noninstancedOffset += attrib->nFloats * sizeof(GLfloat);
        }
    }
}


MeshVertexFormat MeshVertexFormat::Default(unsigned int nBones, bool instancedColor, bool instancedTextureZ) {
    bool animations = nBones != 0;
    return MeshVertexFormat({
        .position = VertexAttribute {.nFloats = 3, .instanced = false},
        .textureUV = VertexAttribute {.nFloats = 2, .instanced = false},   
        .textureZ = VertexAttribute {.nFloats = 1, .instanced = instancedTextureZ},
        .color = VertexAttribute {.nFloats = 4, .instanced = instancedColor}, 
        .modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
        .normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true},
        .normal = VertexAttribute {.nFloats = 3, .instanced = false},   
        .tangent = VertexAttribute {.nFloats = 3, .instanced = false},
        .arbitrary1 = animations ? std::optional(VertexAttribute {.nFloats = 4, .instanced = false, .integer = true}) : std::nullopt, // bone ids
        .arbitrary2 = animations ? std::optional(VertexAttribute {.nFloats = 4, .instanced = false}) : std::nullopt, // bone weights
    }, animations, nBones);
}

// noninstanced (XYZ, TextureXY).
// instanced: model matrix, normal matrix (so texture can be rotated w/o problems), rgba, textureZ
MeshVertexFormat MeshVertexFormat::DefaultGui() {
    return MeshVertexFormat({
            .position = VertexAttribute {.offset = 0, .nFloats = 3, .instanced = false},
            .textureUV = VertexAttribute {.offset = sizeof(glm::vec3), .nFloats = 2, .instanced = false},   
            .textureZ = VertexAttribute {.offset = (sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + sizeof(glm::vec4)), .nFloats = 1, .instanced = true},
            .color = VertexAttribute {.offset = sizeof(glm::mat4x4) + sizeof(glm::mat3x3), .nFloats = 4, .instanced = true}, 
            .modelMatrix = VertexAttribute {.offset = 0, .nFloats = 16, .instanced = true},
            .normalMatrix = VertexAttribute {.offset = sizeof(glm::mat4x4), .nFloats = 9, .instanced = true},
            .normal = std::nullopt,   
            .tangent = std::nullopt,
            .arbitrary1 = VertexAttribute {.offset = sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + sizeof(glm::vec4) + sizeof(float), .nFloats = 1, .instanced = true}
    });
}

void MeshVertexFormat::HandleAttribute(GLuint& vaoId, const std::optional<VertexAttribute>& attribute, const GLuint attributeName, bool justInstanced, unsigned int instancedSize, unsigned int nonInstancedSize) const {
    if (attribute.has_value() && attribute->instanced == justInstanced) {
        Assert(attribute->nFloats > 0);
        // std::cout << "Attribute with name " << attributeName << " has " << attribute->nFloats << " floats.\n";
        Assert(attribute->nFloats <= 4 || attribute->nFloats == 9 || attribute->nFloats == 12 || attribute->nFloats == 16);

        unsigned int nFloats = attribute->nFloats;
        unsigned int nAttributes = 1;
        if (nFloats > 4) {
            if (nFloats == 9) {
                nFloats = 3;
                nAttributes = 3;
            }
            else {
                nFloats = 4;
                nAttributes = attribute->nFloats/4;
            }
        }

        // make sure a vbo is bound
        GLint array = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array); 
        Assert(array != 0); 

        // because each attribute name can only have up to 4 floats in OpenGL, we do a for loop to create more as needed.
        for (unsigned int i = 0; i < nAttributes; i++) {
            
            // Tell OpenGL this vertex attribute exists with the given name (shaders use this name to access the vertex attribute)
            glEnableVertexAttribArray(attributeName + i); 
            
            // Associate data with the VAO and describe format of mesh data
            if (attribute->integer) {
                glVertexAttribIPointer(attributeName + i, nFloats, GL_INT, attribute->instanced ? instancedSize : nonInstancedSize, (void*)(size_t)(attribute->offset + (i * nFloats * sizeof(GLint))));
            }
            else {
                glVertexAttribPointer(attributeName + i, nFloats, GL_FLOAT, false, attribute->instanced ? instancedSize : nonInstancedSize, (void*)(size_t)(attribute->offset + (i * nFloats * sizeof(GLfloat)))); // ignore the warning, this is completely fine
            }
            glVertexAttribDivisor(attributeName + i, attribute->instanced ? 1 : 0); // attribDivisor is whether the vertex attribute is instanced or not.
        }
        
    }
}

bool MeshVertexFormat::operator==(const MeshVertexFormat& other) const {
    for (unsigned int i = 0; i < N_ATTRIBUTES; i++) {
        if (vertexAttributes[i] != other.vertexAttributes[i]) {
            return false;
        }
    }
    return true;
}

void MeshVertexFormat::SetNonInstancedVaoVertexAttributes(GLuint& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const {
    // std::cout << "Pos:\n";
    HandleAttribute(vaoId, attributes.position, MeshVertexFormat::POS_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "Color:\n"; 
    HandleAttribute(vaoId, attributes.color, MeshVertexFormat::COLOR_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "Normal:\n";
    HandleAttribute(vaoId, attributes.normal, MeshVertexFormat::NORMAL_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "Tangent:\n";
    HandleAttribute(vaoId, attributes.tangent, MeshVertexFormat::TANGENT_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "UV:\n";
    HandleAttribute(vaoId, attributes.textureUV, MeshVertexFormat::TEXTURE_UV_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "TZ:\n";
    HandleAttribute(vaoId, attributes.textureZ, MeshVertexFormat::TEXTURE_Z_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "ModelM:\n";
    HandleAttribute(vaoId, attributes.modelMatrix, MeshVertexFormat::MODEL_MATRIX_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);
    // std::cout << "NormalM:\n";
    HandleAttribute(vaoId, attributes.normalMatrix, MeshVertexFormat::NORMAL_MATRIX_ATTRIBUTE_NAME, false, instancedSize, nonInstancedSize);

    HandleAttribute(vaoId, attributes.arbitrary1, MeshVertexFormat::ARBITRARY_ATTRIBUTE_1_NAME, false, instancedSize, nonInstancedSize);
    HandleAttribute(vaoId, attributes.arbitrary2, MeshVertexFormat::ARBITRARY_ATTRIBUTE_2_NAME, false, instancedSize, nonInstancedSize);
}

void MeshVertexFormat::SetInstancedVaoVertexAttributes(GLuint& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const {
    // std::cout << "Position:\n";
    HandleAttribute(vaoId, attributes.position, MeshVertexFormat::POS_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "Color:\n";
    HandleAttribute(vaoId, attributes.color, MeshVertexFormat::COLOR_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "Normal:\n";
    HandleAttribute(vaoId, attributes.normal, MeshVertexFormat::NORMAL_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "Tangent:\n";
    HandleAttribute(vaoId, attributes.tangent, MeshVertexFormat::TANGENT_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "UV:\n";
    HandleAttribute(vaoId, attributes.textureUV, MeshVertexFormat::TEXTURE_UV_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "TZ:\n";
    HandleAttribute(vaoId, attributes.textureZ, MeshVertexFormat::TEXTURE_Z_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "ModelM:\n";
    HandleAttribute(vaoId, attributes.modelMatrix, MeshVertexFormat::MODEL_MATRIX_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);
    // std::cout << "NormalM:\n";
    HandleAttribute(vaoId, attributes.normalMatrix, MeshVertexFormat::NORMAL_MATRIX_ATTRIBUTE_NAME, true, instancedSize, nonInstancedSize);

    HandleAttribute(vaoId, attributes.arbitrary1, MeshVertexFormat::ARBITRARY_ATTRIBUTE_1_NAME, true, instancedSize, nonInstancedSize);
    HandleAttribute(vaoId, attributes.arbitrary2, MeshVertexFormat::ARBITRARY_ATTRIBUTE_2_NAME, true, instancedSize, nonInstancedSize);
}

// engine calls this to get mesh from an object's meshId
std::shared_ptr<Mesh>& Mesh::Get(unsigned int meshId) {
    Assert(MeshGlobals::Get().LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
    return MeshGlobals::Get().LOADED_MESHES[meshId];
}

// // Scales the vertex positions by the given 
// glm::vec3 NormalizeVertices(std::vector<GLfloat> & verts) {

// }

//std::shared_ptr<Mesh> Mesh::FromVertices(const std::vector<GLfloat>& verts, const std::vector<GLuint> &indies, const MeshCreateParams& params) {
//    unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
//    auto ptr = std::shared_ptr<Mesh>(new Mesh(verts, indies, params));
//    MeshGlobals::Get().LOADED_MESHES[meshId] = ptr;
//    return ptr;
//}





// TODO: really bad right now
// std::shared_ptr<Mesh> Mesh::FromText(const std::string &text, const Texture &font, const MeshVertexFormat& vertexFormat, const bool isDynamic) {

//     Assert(vertexFormat.attributes.position.has_value() && vertexFormat.attributes.position->nFloats >= 2);
//     Assert(vertexFormat.attributes.textureUV.has_value() && vertexFormat.attributes.textureUV->nFloats >= 2);
//     Assert(font.usage == Texture::FontMap);
//     Assert(font.glyphs.has_value());

//     std::vector<GLfloat> vertices;
//     std::vector<GLuint> indices;

//     TextMeshFromText(text, font, vertexFormat, vertices, indices);

//     unsigned int i = LAST_MESH_ID;
//     auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, isDynamic, vertexFormat, 1, false, true));
//     LOADED_MESHES[i] = ptr;
//     return ptr;
// }

//std::shared_ptr<Mesh> Mesh::Text(const MeshCreateParams& params) {
//    unsigned int i = MeshGlobals::Get().LAST_MESH_ID;
//    auto realParams = MeshCreateParams {.meshVertexFormat = params.meshVertexFormat, .expectedCount = 1, .normalizeSize = false, .dynamic = true};
//    auto ptr = std::shared_ptr<Mesh>(new Mesh(Mesh::Square()->meshVertices, Mesh::Square()->meshIndices, realParams, true));
//    MeshGlobals::Get().LOADED_MESHES[i] = ptr;
//    return ptr;
//}



// std::shared_ptr<Mesh> Mesh::FromFile(const std::string& path, const MeshCreateParams& params) {
//     // Load file
//     auto config = tinyobj::ObjReaderConfig();
//     config.triangulate = true;
//     config.vertex_color = !params.meshVertexFormat.attributes.color->instanced;
//     bool success = MeshGlobals::Get().OBJ_LOADER.ParseFromFile(path, config);
//     if (!success) {
//         std::printf("Mesh::FromFile failed to load %s because %s", path.c_str(), MeshGlobals::Get().OBJ_LOADER.Error().c_str());
//         abort();
//     }

//     auto nFloatsPerVertex = params.meshVertexFormat.GetNonInstancedVertexSize()/sizeof(GLfloat);

//     auto shape = MeshGlobals::Get().OBJ_LOADER.GetShapes().at(0);
//     //auto material = OBJ_LOADER.GetMaterials().at(0);
//     auto attrib = MeshGlobals::Get().OBJ_LOADER.GetAttrib();

//     std::vector<GLfloat> & positions = attrib.vertices;
//     std::vector<GLfloat> & texcoordsXY = attrib.texcoords;
//     std::vector<GLfloat> & normals = attrib.normals;
//     std::vector<GLfloat> & colors = attrib.colors;

//     // take all the seperate colors, positions, etc. and put them in a single interleaved vertices thing with one indices
//     std::unordered_map<std::tuple<GLuint, GLuint, GLuint>, GLuint, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> objIndicesToGlIndices; // tuple is (posIndex, texXyIndex, normalIndex)
//     std::vector<GLuint> indices;
//     std::vector<GLfloat> vertices;



//     unsigned int currentVertex = 0;
//     for (auto & index : shape.mesh.indices) {
//         auto indexTuple = std::make_tuple(index.vertex_index, index.texcoord_index, index.normal_index);

//         // if we have this exact vertex already, just add another index for it, otherwise append a new vertex
//         if (objIndicesToGlIndices.count(indexTuple) == 0) {

//             objIndicesToGlIndices[indexTuple] = vertices.size()/nFloatsPerVertex;
//             indices.push_back(currentVertex);

//             // position
//             if (params.meshVertexFormat.attributes.position.has_value() && !params.meshVertexFormat.attributes.position->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.position->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.position->offset/sizeof(GLfloat) + i, vertices) = (positions[index.vertex_index * 3 + i]);
//                 }
//             }

//             // uv
//             if (params.meshVertexFormat.attributes.textureUV.has_value() && !params.meshVertexFormat.attributes.textureUV->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.textureUV->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + i, vertices) = (texcoordsXY[index.texcoord_index * 2 + i]);
//                 }
//             }
            
//             // normal
//             if (params.meshVertexFormat.attributes.normal.has_value() && !params.meshVertexFormat.attributes.normal->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.normal->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.normal->offset/sizeof(GLfloat) + i, vertices) = (normals[index.normal_index * 3 + i]);
//                 }
//             }

//             // tangent
//             if (params.meshVertexFormat.attributes.tangent.has_value() && !params.meshVertexFormat.attributes.tangent->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.tangent->nFloats; i++) {
//                     // we just set these to 0 for now, they're calculating for real at a later step
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }
            
//             // textureZ
//             if (params.meshVertexFormat.attributes.textureZ.has_value() && !params.meshVertexFormat.attributes.textureZ->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.textureZ->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.textureZ->offset/sizeof(GLfloat) + i, vertices) = (params.textureZ);
//                 }
//             }

//             // color
//             if (params.meshVertexFormat.attributes.color.has_value() && !params.meshVertexFormat.attributes.color->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.color->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.color->offset/sizeof(GLfloat) + i, vertices) = (i == 3 ? params.opacity : colors[index.vertex_index * 3 + i]);
//                 }
//             }

//             // model matrix, though i fear for your sanity if this isn't instanced
//             if (params.meshVertexFormat.attributes.modelMatrix.has_value() && !params.meshVertexFormat.attributes.modelMatrix->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.modelMatrix->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.modelMatrix->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }

//             // normal matrix, again why would you not instance this
//             if (params.meshVertexFormat.attributes.normalMatrix.has_value() && !params.meshVertexFormat.attributes.normalMatrix->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.normalMatrix->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.normalMatrix->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }

//             if (params.meshVertexFormat.attributes.arbitrary1.has_value() && !params.meshVertexFormat.attributes.arbitrary1->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.arbitrary1->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.arbitrary1->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }

//             if (params.meshVertexFormat.attributes.arbitrary2.has_value() && !params.meshVertexFormat.attributes.arbitrary2->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.arbitrary2->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.arbitrary2->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }
            
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.textureUV->offset/sizeof(GLfloat), vertices) = (texcoordsXY[index.texcoord_index * 2]); 
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.textureUV->offset/sizeof(GLfloat) + 1, vertices) = (texcoordsXY[index.texcoord_index * 2 + 1]);
            
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.normal->offset/sizeof(GLfloat), vertices) = (normals[index.normal_index * 3]);
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.normal->offset/sizeof(GLfloat) + 1, vertices) = (normals[index.normal_index * 3 + 1]);
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.normal->offset/sizeof(GLfloat) + 2, vertices) = (normals[index.normal_index * 3 + 2]);


//             // if (params.meshVertexFormat.color && !params.meshVertexFormat.color->instanced) {
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3]);
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3 + 1]);
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3 + 2]);
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (meshTransparency);
//             // }

//             currentVertex += 1;
//         }
//         else {
//             indices.push_back(objIndicesToGlIndices[indexTuple]);
//         }
//     }

//     // calculate tangent vectors
//     if (params.meshVertexFormat.attributes.tangent.has_value() && params.meshVertexFormat.attributes.tangent->instanced == false) {

//         std::vector<GLfloat> tangents;
        
//         // to calculate tangent vectors, we must put a few restrictions on the meshVertexFormat.
//         Assert(params.meshVertexFormat.attributes.position.has_value() && params.meshVertexFormat.attributes.position->nFloats >= 3 && params.meshVertexFormat.attributes.position->instanced == false);
//         Assert(params.meshVertexFormat.attributes.textureUV.has_value() && params.meshVertexFormat.attributes.textureUV->nFloats >= 2 && params.meshVertexFormat.attributes.textureUV->instanced == false);
//         Assert(params.meshVertexFormat.attributes.normal.has_value() && params.meshVertexFormat.attributes.normal->nFloats >= 3 && params.meshVertexFormat.attributes.normal->instanced == false);
//         Assert(params.meshVertexFormat.attributes.tangent->nFloats >= 3);

//         for (unsigned int triangleIndex = 0; triangleIndex < indices.size()/3; triangleIndex++) {
//             glm::vec3 points[3];
//             glm::vec2 texCoords[3];
//             glm::vec3 l_normals[3]; // l_ to avoid shadowing
//             for (unsigned int j = 0; j < 3; j++) {
//                 auto indexIntoIndices = triangleIndex * 3 + j;
//                 //std::cout << " i = " << indexIntoIndices << "nfloats = " << nFloatsPerVertex << " actual index = " << indices.at(indexIntoIndices) <<  " \n";
//                 auto vertexIndex = indices.at(indexIntoIndices);
//                 //std::cout << "thing in indices  was " << vertexIndex << " \n"; 
//                 points[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.position->offset/sizeof(GLfloat)));
//                 texCoords[j] = glm::make_vec2(&vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.textureUV->offset/sizeof(GLfloat)));
//                 l_normals[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.normal->offset/sizeof(GLfloat)));
//             }
            
//             glm::vec3 edge1 = points[1] - points[0];
//             glm::vec3 edge2 = points[2] - points[0];
//             glm::vec2 deltaUV1 = texCoords[1] - texCoords[0];
//             glm::vec2 deltaUV2 = texCoords[2] - texCoords[0]; 
    
//             float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);


//             auto atangentX = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
//             auto atangentY = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
//             auto atangentZ = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

//             for (unsigned int i2 = 0; i2 < 3; i2++) {
//                 auto indexIntoIndices = triangleIndex * 3 + i2;
//                 auto vertexIndex = indices.at(indexIntoIndices);
//                 vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat)) = atangentX;
//                 vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + 1) = atangentY;
//                 vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + 2) = atangentZ;
//             }
            
//         }
//     }
    
//     unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
//     auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, params));
//     MeshGlobals::Get().LOADED_MESHES[meshId] = ptr;
//     return ptr;
// }


void Mesh::Unload(int meshId) {
    Assert(MeshGlobals::Get().LOADED_MESHES.count(meshId) != 0 && "Mesh::Unload() was given an invalid meshId.");
    MeshGlobals::Get().LOADED_MESHES.erase(meshId);
}

// unsigned int MaxBonesFromAnims(std::optional<std::vector<Animation>> anims) {
//     if (!anims.has_value()) {return 0;}
//     unsigned int greatest = 0;
//     for (auto & anim: anims.value()) {
//         if (meshBones.size() > greatest) {
//             greatest = meshBones.size();
//         } 
//     }
//     return greatest;
// }

Mesh::Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, const MeshCreateParams& params, bool dynamic, bool fromText, std::optional<std::vector<Bone>> bonez, std::optional<std::vector<Animation>> anims, unsigned int rootBoneIndex):
dynamic(dynamic),
meshId(MeshGlobals::Get().LAST_MESH_ID++),
instanceCount(params.expectedCount),
nonInstancedVertexSize(params.meshVertexFormat.value().GetNonInstancedVertexSize()),
instancedVertexSize(params.meshVertexFormat.value().GetInstancedVertexSize()),
vertexFormat(params.meshVertexFormat.value()),
wasCreatedFromText(fromText),
meshVertices(verts),
meshIndices(indies),
meshBones(bonez),
meshAnimations(anims),
rootBoneId(rootBoneIndex)
{    

    if (params.normalizeSize) {
        NormalizePositions();
    }

    if (meshAnimations) {
        Assert(meshBones->size() <= vertexFormat.maxBones);
        // Assert(meshAnimations->size() > 0);
        // TODO: ???
        // float nBones = meshAnimations->at(0).bones.size();
        // for (auto & anim: *meshAnimations) {
        //     if (anim.bones.size() != nBones) { // TODO: am i right here?????
        //         DebugLogError("The animation ", anim.name, " has ", anim.bones.size(), " bones, but at least one other animation on this mesh has ", nBones, " bones. All animations on any given mesh must have the same number of bones.");
        //     } 
        // }
    }
    
}

void Mesh::NormalizePositions() {
    Assert(vertexFormat.attributes.position.has_value() && !vertexFormat.attributes.position->instanced && (vertexFormat.attributes.position->nFloats == 2 || vertexFormat.attributes.position->nFloats == 3));

    // find mesh extents
    float minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
    for (unsigned int i = 0; i < vertices.size(); i++) {
        const float & v = vertices[i];
        unsigned int remainder = (i % (nonInstancedVertexSize/sizeof(GLfloat))) - (vertexFormat.attributes.position->offset/sizeof(GLfloat));
        
        if (remainder == 0) {
            minX = std::min(minX, v);
            maxX = std::max(maxX, v);
        }
        else if (remainder == 1) {
            minY = std::min(minY, v);
            maxY = std::max(maxY, v);
        }
        else if (remainder == 2 && vertexFormat.attributes.position->nFloats == 3) {
            minZ = std::min(minZ, v);
            maxZ = std::max(maxZ, v);
        }
    }

    // scale mesh by extents
    for (unsigned int i = 0; i < vertices.size(); i++) {
        float & v = meshVertices[i];
        unsigned int remainder = (i % (nonInstancedVertexSize/sizeof(GLfloat))) - (vertexFormat.attributes.position->offset/sizeof(GLfloat));
        if (remainder == 0) {
            v = 1.0f*(v-minX)/(maxX-minX) - 0.5f; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (remainder == 1) {
            v = 1.0f*(v-minY)/(maxY-minY) - 0.5f; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (remainder == 2 && vertexFormat.attributes.position->nFloats == 3) {
            v = 1.0f*(v-minZ)/(maxZ-minZ) - 0.5f; // i don't really know how this bit works i got it from stack overflow and modified it
        }
    }

    originalSize = {maxX - minX, maxY - minY, maxZ - minZ};
}

std::pair<std::vector<GLfloat>&, std::vector<GLuint>&> Mesh::StartModifying() {
    Assert(dynamic == true);
    return {meshVertices, meshIndices};
}

void Mesh::StopModifying(bool normalizeSize) {
    Assert(dynamic == true);

    if (normalizeSize) {
        NormalizePositions();
    }
 
    // std::cout << "MODIFICATION HALTED\n";
    // std::cout << "There are " << GraphicsEngine::Get().dynamicMeshUsers.size() << " dynamic mesh users.\n";

    // keep in mind that same mesh could be in multiple different meshpools because different materials/shaders 

    std::vector<std::pair<unsigned int, unsigned int>> modifiedMeshpoolIds; // pairs of <poolid, poolslot> make sure we don't waste perf. resetting the same meshpool multiple times

    // For every object that uses this dynamic mesh...
    if (GraphicsEngine::Get().dynamicMeshUsers.contains(meshId)) {
        for (auto & renderComponent : GraphicsEngine::Get().dynamicMeshUsers.at(meshId)) {

            // if the object hasn't been added to a meshpool yet (because it was created this frame), we needn't do anything.
            if (renderComponent->meshpoolId == -1) {
                continue;
            }

            // std::cout << "Using " << renderComponent->shaderProgramId << " " << renderComponent->materialId << " " << renderComponent->meshpoolId << ".\n";
            Meshpool& pool = *GraphicsEngine::Get().meshpools.at(renderComponent->shaderProgramId).at(renderComponent->materialId).at(renderComponent->meshpoolId);


            
            // If the vertex/index counts didn't change enough to make the mesh not fit in the pool, just refill the meshpool's slot.
            // std::cout << "Pool holds " << pool.meshVerticesSize << "bytes, but the mesh is " << vertices.size() * sizeof(GLfloat) << "bytes.\n";
            if (pool.meshVerticesSize >= vertices.size() * sizeof(GLfloat) && pool.meshIndicesSize >= indices.size() * sizeof(GLuint)) {

                // if we already did that, move onto the next object.
                if (std::find(modifiedMeshpoolIds.begin(), modifiedMeshpoolIds.end(), std::make_pair<unsigned int, unsigned int>(renderComponent->meshpoolId, renderComponent->meshpoolSlot)) != modifiedMeshpoolIds.end()) {
                    continue;
                }

                // std::cout << "Filling slot for dynamic mesh update.\n";

                // instanceCount argument is meaningless when modifying == true.
                pool.FillSlot(meshId, renderComponent->meshpoolSlot, 0, true);
                modifiedMeshpoolIds.push_back({renderComponent->meshpoolId, renderComponent->meshpoolSlot});
            }

            // Otherwise, we have to remove every object using this mesh/meshpool combo from its meshpool, and find a new pool for it.
            else {
                // todo: could potentially do some crazy optimization here by manually wiping the mesh from the pool then directly setting pool ids for new pool?
                // TODO: making component no longer in a meshpool could break stuff that calls methods of rendercomponent before it's readded
                // low priority in any case

                std::cout << "Removing/readding object for dynamic mesh update.\n";

                pool.RemoveObject(renderComponent->meshpoolSlot, renderComponent->meshpoolInstance);
                GraphicsEngine::Get().AddObject(renderComponent->shaderProgramId, renderComponent->materialId, meshId, renderComponent);
            }
            }
    }
    
    
}

void Mesh::Remesh(const MeshProvider& provider)
{
    auto& [v, i] = StartModifying();
    auto& [newV, newI] = provider.GetMesh();
    v = newV;
    i = newI;
    StopModifying(true);
}
