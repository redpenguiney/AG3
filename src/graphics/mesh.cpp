#include "mesh.hpp"
#include "texture.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>
#include "../../external_headers/GLM/gtc/type_ptr.hpp"
#define TINYOBJLOADER_IMPLEMENTATION // what kind of library makes you have to ask it to actually implement the functions???
#include "../../external_headers/tinyobjloader/tiny_obj_loader.h"
#include "../utility/let_me_hash_a_tuple.cpp"
#include "engine.hpp"

// helper thing for Mesh::FromFile() and TextMeshFromText() that will expand the vector so that it contains index if needed, then return vector.at(index)
template<typename T>
typename std::vector<T>::reference vectorAtExpanding(unsigned int index, std::vector<T>& vector) {
    if (vector.size() <= index) {
        vector.resize(index + 1, 6.66);
    }
    return vector.at(index);
}

std::shared_ptr<Mesh> Mesh::Square() {
    static std::vector<GLfloat> squareVerts = {
        -1.0, -1.0,   0.0, 0.0, 
         1.0, -1.0,   1.0, 0.0,
         1.0,  1.0,   1.0, 1.0,
        -1.0,  1.0,   0.0, 1.0,
         };

    static auto m = Mesh::FromVertices(squareVerts, {0, 1, 2, 0, 2, 3}, false, MeshVertexFormat::DefaultGui(), 1.0, false);
    return m;
}

void TextMeshFromText(const std::string &text, const Texture &font, const MeshVertexFormat& vertexFormat, std::vector<GLfloat>& vertices, std::vector<GLuint>& indices) {
    GLfloat currentX = 0;
    GLfloat currentY = 0;

    vertices.clear();
    indices.clear();

    unsigned int vertexIndex = 0;
    unsigned int vertexSize = vertexFormat.GetNonInstancedVertexSize()/sizeof(GLfloat);

    for (const char & c: text) {
        if (c == '\n') {
            currentY += font.lineSpacing;
            currentX = 0;
            continue;
        }

        const auto & glyph = font.glyphs->at(c);

        // make sure the font actually has the letter we want (TODO give a default character instead)
        assert(font.glyphs->count(c));

        // based on https://learnopengl.com/In-Practice/Text-Rendering 
        GLfloat characterX = currentX + glyph.bearingX;
        GLfloat width = glyph.width;
        GLfloat characterY = currentY - glyph.bearingY;
        GLfloat height = glyph.height;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat), vertices) = characterX;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 1, vertices) = characterY + height;
        if (vertexFormat.attributes.position->nFloats > 2) {
            assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat), vertices) = glyph.leftUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + 1, vertices) = glyph.bottomUv;
        indices.push_back(vertexIndex);
        vertexIndex += 1;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat), vertices) = characterX + width;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 1, vertices) = characterY + height;
        if (vertexFormat.attributes.position->nFloats > 2) {
            assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat), vertices) = glyph.rightUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + 1, vertices) = glyph.bottomUv;
        indices.push_back(vertexIndex);

        vertexIndex += 1;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat), vertices) = characterX + width;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 1, vertices) = characterY;
        if (vertexFormat.attributes.position->nFloats > 2) {
            assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat), vertices) = glyph.rightUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + 1, vertices) = glyph.topUv;
        indices.push_back(vertexIndex);
        indices.push_back(vertexIndex - 2);
        indices.push_back(vertexIndex);
        vertexIndex += 1;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat), vertices) = characterX;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 1, vertices) = characterY;
        if (vertexFormat.attributes.position->nFloats > 2) {
            assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset/sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat), vertices) = glyph.leftUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + 1, vertices) = glyph.topUv;
        indices.push_back(vertexIndex);
        vertexIndex += 1;

        currentX += glyph.advance;
    }
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

// noninstanced (XYZ, TextureXY, NormalXYZ, TangentXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ).
// instanced: model matrix, normal matrix, rgba if instanced, textureZ if instanced
MeshVertexFormat MeshVertexFormat::Default(bool instancedColor, bool instancedTextureZ) {
    return MeshVertexFormat {
        .attributes = {
            .position = VertexAttribute {.offset = 0, .nFloats = 3, .instanced = false},
            .textureUV = VertexAttribute {.offset = sizeof(glm::vec3), .nFloats = 2, .instanced = false},   
            .textureZ = VertexAttribute {.offset = (unsigned short)(instancedTextureZ ? (sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + (instancedColor ? sizeof(glm::vec4) : 0)) : (3 * sizeof(glm::vec3) + sizeof(glm::vec2) + (!instancedColor ? sizeof(glm::vec4) : 0))), .nFloats = 1, .instanced = instancedTextureZ},
            .color = VertexAttribute {.offset = (unsigned short)(instancedColor ? (sizeof(glm::mat4x4) + sizeof(glm::mat3x3)) : (3 * sizeof(glm::vec3) + sizeof(glm::vec2))), .nFloats = 4, .instanced = instancedColor}, 
            .modelMatrix = VertexAttribute {.offset = 0, .nFloats = 16, .instanced = true},
            .normalMatrix = VertexAttribute {.offset = sizeof(glm::mat4x4), .nFloats = 9, .instanced = true},
            .normal = VertexAttribute {.offset = sizeof(glm::vec3) + sizeof(glm::vec2), .nFloats = 3, .instanced = false},   
            .tangent = VertexAttribute {.offset = 2 * sizeof(glm::vec3) + sizeof(glm::vec2), .nFloats = 3, .instanced = false},
        }
    };
}

// noninstanced (XY, TextureXY).
// instanced: model matrix, normal matrix (so texture can be rotated w/o problems), rgba, textureZ
MeshVertexFormat MeshVertexFormat::DefaultGui() {
    return MeshVertexFormat {
        .attributes = {
            .position = VertexAttribute {.offset = 0, .nFloats = 2, .instanced = false},
            .textureUV = VertexAttribute {.offset = sizeof(glm::vec2), .nFloats = 2, .instanced = false},   
            .textureZ = VertexAttribute {.offset = (sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + sizeof(glm::vec4)), .nFloats = 1, .instanced = true},
            .color = VertexAttribute {.offset = sizeof(glm::mat4x4) + sizeof(glm::mat3x3), .nFloats = 4, .instanced = true}, 
            .modelMatrix = VertexAttribute {.offset = 0, .nFloats = 16, .instanced = true},
            .normalMatrix = VertexAttribute {.offset = sizeof(glm::mat4x4), .nFloats = 9, .instanced = true},
            .normal = std::nullopt,   
            .tangent = std::nullopt,
        }
    };
}

void MeshVertexFormat::HandleAttribute(GLuint& vaoId, const std::optional<VertexAttribute>& attribute, const GLuint attributeName, bool justInstanced, unsigned int instancedSize, unsigned int nonInstancedSize) const {
    if (attribute.has_value() && attribute->instanced == justInstanced) {
        assert(attribute->nFloats > 0);
        // std::cout << "Attribute with name " << attributeName << " has " << attribute->nFloats << " floats.\n";
        assert(attribute->nFloats <= 4 || attribute->nFloats == 9 || attribute->nFloats == 12 || attribute->nFloats == 16);

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

        GLint array = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array); // make sure a vbo is bound
        assert(array != 0); 

        // because each attribute name can only have up to 4 floats in OpenGL, we do a for loop to create more as needed.
        for (unsigned int i = 0; i < nAttributes; i++) {
            
            // Tell OpenGL this vertex attribute exists with the given name (shaders use this name to access the vertex attribute)
            glEnableVertexAttribArray(attributeName + i); 
            
            // Associate data with the VAO and describe format of mesh data
            glVertexAttribPointer(attributeName + i, nFloats, GL_FLOAT, false, attribute->instanced ? instancedSize : nonInstancedSize, (void*)(size_t)(attribute->offset + (i * nFloats * sizeof(GLfloat)))); // ignore the warning, this is completely fine
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
}

// engine calls this to get mesh from an object's meshId
std::shared_ptr<Mesh>& Mesh::Get(unsigned int meshId) {
    assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
    return LOADED_MESHES[meshId];
}

// // Scales the vertex positions by the given 
// glm::vec3 NormalizeVertices(std::vector<GLfloat> & verts) {

// }

std::shared_ptr<Mesh> Mesh::FromVertices(const std::vector<GLfloat>& verts, const std::vector<GLuint> &indies, const bool isDynamic, const MeshVertexFormat& meshVertexFormat, unsigned int expectedCount, bool normalizeSize) {
    unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
    auto ptr = std::shared_ptr<Mesh>(new Mesh(verts, indies, isDynamic, meshVertexFormat, expectedCount, normalizeSize));
    LOADED_MESHES[meshId] = ptr;
    return ptr;
}





// TODO: really bad right now
std::shared_ptr<Mesh> Mesh::FromText(const std::string &text, const Texture &font, const MeshVertexFormat& vertexFormat, const bool isDynamic) {

    assert(vertexFormat.attributes.position.has_value() && vertexFormat.attributes.position->nFloats >= 2);
    assert(vertexFormat.attributes.textureUV.has_value() && vertexFormat.attributes.textureUV->nFloats >= 2);
    assert(font.usage == Texture::FontMap);
    assert(font.glyphs.has_value());

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    TextMeshFromText(text, font, vertexFormat, vertices, indices);

    unsigned int i = LAST_MESH_ID;
    auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, isDynamic, vertexFormat, 1, true, true));
    LOADED_MESHES[i] = ptr;
    return ptr;
}

std::shared_ptr<Mesh> Mesh::FromFile(const std::string& path, const MeshVertexFormat& meshVertexFormat, float textureZ, unsigned int meshTransparency, unsigned int expectedCount, bool normalizeSize, const bool isDynamic) {
    // Load file
    auto config = tinyobj::ObjReaderConfig();
    config.triangulate = true;
    config.vertex_color = !meshVertexFormat.attributes.color->instanced;
    bool success = OBJ_LOADER.ParseFromFile(path, config);
    if (!success) {
        std::printf("Mesh::FromFile failed to load %s because %s", path.c_str(), OBJ_LOADER.Error().c_str());
        abort();
    }

    auto nFloatsPerVertex = meshVertexFormat.GetNonInstancedVertexSize()/sizeof(GLfloat);

    auto shape = OBJ_LOADER.GetShapes().at(0);
    //auto material = OBJ_LOADER.GetMaterials().at(0);
    auto attrib = OBJ_LOADER.GetAttrib();

    std::vector<GLfloat> & positions = attrib.vertices;
    std::vector<GLfloat> & texcoordsXY = attrib.texcoords;
    std::vector<GLfloat> & normals = attrib.normals;
    std::vector<GLfloat> & colors = attrib.colors;

    // take all the seperate colors, positions, etc. and put them in a single interleaved vertices thing with one indices
    std::unordered_map<std::tuple<GLuint, GLuint, GLuint>, GLuint, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> objIndicesToGlIndices; // tuple is (posIndex, texXyIndex, normalIndex)
    std::vector<GLuint> indices;
    std::vector<GLfloat> vertices;



    unsigned int currentVertex = 0;
    for (auto & index : shape.mesh.indices) {
        auto indexTuple = std::make_tuple(index.vertex_index, index.texcoord_index, index.normal_index);

        // if we have this exact vertex already, just add another index for it, otherwise append a new vertex
        if (objIndicesToGlIndices.count(indexTuple) == 0) {

            objIndicesToGlIndices[indexTuple] = vertices.size()/nFloatsPerVertex;
            indices.push_back(currentVertex);

            // position
            if (meshVertexFormat.attributes.position.has_value() && !meshVertexFormat.attributes.position->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.position->nFloats; i++) {
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.position->offset/sizeof(GLfloat) + i, vertices) = (positions[index.vertex_index * 3 + i]);
                }
            }

            // uv
            if (meshVertexFormat.attributes.textureUV.has_value() && !meshVertexFormat.attributes.textureUV->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.textureUV->nFloats; i++) {
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + i, vertices) = (texcoordsXY[index.texcoord_index * 2 + i]);
                }
            }
            
            // normal
            if (meshVertexFormat.attributes.normal.has_value() && !meshVertexFormat.attributes.normal->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.normal->nFloats; i++) {
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.normal->offset/sizeof(GLfloat) + i, vertices) = (normals[index.normal_index * 3 + i]);
                }
            }

            // tangent
            if (meshVertexFormat.attributes.tangent.has_value() && !meshVertexFormat.attributes.tangent->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.tangent->nFloats; i++) {
                    // we just set these to 0 for now, they're calculating for real at a later step
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + i, vertices) = (0);
                }
            }
            
            // textureZ
            if (meshVertexFormat.attributes.textureZ.has_value() && !meshVertexFormat.attributes.textureZ->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.textureZ->nFloats; i++) {
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.textureZ->offset/sizeof(GLfloat) + i, vertices) = (textureZ);
                }
            }

            // color
            if (meshVertexFormat.attributes.color.has_value() && !meshVertexFormat.attributes.color->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.color->nFloats; i++) {
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.color->offset/sizeof(GLfloat) + i, vertices) = (i == 3 ? meshTransparency : colors[index.vertex_index * 3 + i]);
                }
            }

            // model matrix, though i fear for your sanity if this isn't instanced
            if (meshVertexFormat.attributes.modelMatrix.has_value() && !meshVertexFormat.attributes.modelMatrix->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.modelMatrix->nFloats; i++) {
                    // we just set these to 0 for now, it's the user's job to set values to them
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.modelMatrix->offset/sizeof(GLfloat) + i, vertices) = (0);
                }
            }

            // normal matrix, again why would you not instance this
            if (meshVertexFormat.attributes.normalMatrix.has_value() && !meshVertexFormat.attributes.normalMatrix->instanced) {
                for (unsigned int i = 0; i < meshVertexFormat.attributes.normalMatrix->nFloats; i++) {
                    // we just set these to 0 for now, it's the user's job to set values to them
                    vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.attributes.normalMatrix->offset/sizeof(GLfloat) + i, vertices) = (0);
                }
            }
            
            // vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.textureUV->offset/sizeof(GLfloat), vertices) = (texcoordsXY[index.texcoord_index * 2]); 
            // vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.textureUV->offset/sizeof(GLfloat) + 1, vertices) = (texcoordsXY[index.texcoord_index * 2 + 1]);
            
            // vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.normal->offset/sizeof(GLfloat), vertices) = (normals[index.normal_index * 3]);
            // vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.normal->offset/sizeof(GLfloat) + 1, vertices) = (normals[index.normal_index * 3 + 1]);
            // vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.normal->offset/sizeof(GLfloat) + 2, vertices) = (normals[index.normal_index * 3 + 2]);


            // if (meshVertexFormat.color && !meshVertexFormat.color->instanced) {
            //     vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3]);
            //     vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3 + 1]);
            //     vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3 + 2]);
            //     vectorAtExpanding(currentVertex * nFloatsPerVertex + meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (meshTransparency);
            // }

            currentVertex += 1;
        }
        else {
            indices.push_back(objIndicesToGlIndices[indexTuple]);
        }
    }

    // calculate tangent vectors
    if (meshVertexFormat.attributes.tangent.has_value() && meshVertexFormat.attributes.tangent->instanced == false) {

        std::vector<GLfloat> tangents;
        
        // to calculate tangent vectors, we must put a few restrictions on the meshVertexFormat.
        assert(meshVertexFormat.attributes.position.has_value() && meshVertexFormat.attributes.position->nFloats >= 3 && meshVertexFormat.attributes.position->instanced == false);
        assert(meshVertexFormat.attributes.textureUV.has_value() && meshVertexFormat.attributes.textureUV->nFloats >= 2 && meshVertexFormat.attributes.textureUV->instanced == false);
        assert(meshVertexFormat.attributes.normal.has_value() && meshVertexFormat.attributes.normal->nFloats >= 3 && meshVertexFormat.attributes.normal->instanced == false);
        assert(meshVertexFormat.attributes.tangent->nFloats >= 3);

        for (unsigned int triangleIndex = 0; triangleIndex < indices.size()/3; triangleIndex++) {
            glm::vec3 points[3];
            glm::vec2 texCoords[3];
            glm::vec3 l_normals[3]; // l_ to avoid shadowing
            for (unsigned int j = 0; j < 3; j++) {
                auto indexIntoIndices = triangleIndex * 3 + j;
                //std::cout << " i = " << indexIntoIndices << "nfloats = " << nFloatsPerVertex << " actual index = " << indices.at(indexIntoIndices) <<  " \n";
                auto vertexIndex = indices.at(indexIntoIndices);
                //std::cout << "thing in indices  was " << vertexIndex << " \n"; 
                points[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + meshVertexFormat.attributes.position->offset/sizeof(GLfloat)));
                texCoords[j] = glm::make_vec2(&vertices.at(nFloatsPerVertex * vertexIndex + meshVertexFormat.attributes.textureUV->offset/sizeof(GLfloat)));
                l_normals[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + meshVertexFormat.attributes.normal->offset/sizeof(GLfloat)));
            }
            
            glm::vec3 edge1 = points[1] - points[0];
            glm::vec3 edge2 = points[2] - points[0];
            glm::vec2 deltaUV1 = texCoords[1] - texCoords[0];
            glm::vec2 deltaUV2 = texCoords[2] - texCoords[0]; 
    
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);


            auto atangentX = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            auto atangentY = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            auto atangentZ = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            for (unsigned int i2 = 0; i2 < 3; i2++) {
                auto indexIntoIndices = triangleIndex * 3 + i2;
                auto vertexIndex = indices.at(indexIntoIndices);
                vertices.at(nFloatsPerVertex * vertexIndex + meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat)) = atangentX;
                vertices.at(nFloatsPerVertex * vertexIndex + meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + 1) = atangentY;
                vertices.at(nFloatsPerVertex * vertexIndex + meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + 2) = atangentZ;
            }
            
        }
    }
    
    unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
    auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, isDynamic, meshVertexFormat, expectedCount, normalizeSize));
    LOADED_MESHES[meshId] = ptr;
    return ptr;
}

// unloads the mesh with the given meshId, freeing its memory.
// you CAN unload a mesh while objects are using it without any issues - a copy of that mesh is still on the gpu.
// you only need to call this function if you are (like for procedural terrain) dynamically loading new meshes
void Mesh::Unload(int meshId) {
    assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Unload() was given an invalid meshId.");
    LOADED_MESHES.erase(meshId);
}

Mesh::Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, const bool isDynamic, const MeshVertexFormat& meshVertexFormat, unsigned int expectedCount, bool normalizePositions, bool fromText):
dynamic(isDynamic),
meshId(LAST_MESH_ID++),
instanceCount(expectedCount),
nonInstancedVertexSize(meshVertexFormat.GetNonInstancedVertexSize()),
instancedVertexSize(meshVertexFormat.GetInstancedVertexSize()),
vertexFormat(meshVertexFormat),
wasCreatedFromText(fromText),
meshVertices(verts),
meshIndices(indies)
{
    if (normalizePositions) {
        NormalizePositions();
    }
    
}

void Mesh::NormalizePositions() {
    assert(vertexFormat.attributes.position.has_value() && !vertexFormat.attributes.position->instanced && (vertexFormat.attributes.position->nFloats == 2 || vertexFormat.attributes.position->nFloats == 3));

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
            v = 1.0*(v-minX)/(maxX-minX) - 0.5; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (remainder == 1) {
            v = 1.0*(v-minY)/(maxY-minY) - 0.5; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (remainder == 2 && vertexFormat.attributes.position->nFloats == 3) {
            v = 1.0*(v-minZ)/(maxZ-minZ) - 0.5; // i don't really know how this bit works i got it from stack overflow and modified it
        }
    }

    originalSize = {maxX - minX, maxY - minY, maxZ - minZ};
}

std::pair<std::vector<GLfloat>&, std::vector<GLuint>&> Mesh::StartModifying() {
    assert(dynamic == true);
    return {meshVertices, meshIndices};
}

void Mesh::StopModifying(bool normalizeSize) {
    assert(dynamic == true);

    if (normalizeSize) {
        NormalizePositions();
    }

    std::cout << "MODIFICATION HALTED\n";
    std::cout << "There are " << GraphicsEngine::Get().dynamicMeshUsers.size() << " dynamic mesh users.\n";

    // keep in mind that same mesh could be in multiple different meshpools because different materials/shaders 

    std::vector<std::pair<unsigned int, unsigned int>> modifiedMeshpoolIds; // pairs of <poolid, poolslot> make sure we don't waste perf. resetting the same meshpool multiple times

    // For every object that uses this dynamic mesh...
    if (GraphicsEngine::Get().dynamicMeshUsers.contains(meshId)) {
        for (auto & renderComponent : GraphicsEngine::Get().dynamicMeshUsers.at(meshId)) {

            // if the object hasn't been added to a meshpool yet (because it was created this frame), we needn't do anything.
            if (renderComponent->meshpoolId == -1) {
                continue;
            }

            std::cout << "Using " << renderComponent->shaderProgramId << " " << renderComponent->materialId << " " << renderComponent->meshpoolId << ".\n";
            Meshpool& pool = *GraphicsEngine::Get().meshpools.at(renderComponent->shaderProgramId).at(renderComponent->materialId).at(renderComponent->meshpoolId);


            
            // If the vertex/index counts didn't change enough to make the mesh not fit in the pool, just refill the meshpool's slot.
            std::cout << "Pool holds " << pool.meshVerticesSize << "bytes, but the mesh is " << vertices.size() * sizeof(GLfloat) << "bytes.\n";
            if (pool.meshVerticesSize >= vertices.size() * sizeof(GLfloat) && pool.meshIndicesSize >= indices.size() * sizeof(GLuint)) {

                // if we already did that, move onto the next object.
                if (std::find(modifiedMeshpoolIds.begin(), modifiedMeshpoolIds.end(), std::make_pair<unsigned int, unsigned int>(renderComponent->meshpoolId, renderComponent->meshpoolSlot)) != modifiedMeshpoolIds.end()) {
                    continue;
                }

                std::cout << "Filling slot for dynamic mesh update.\n";

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