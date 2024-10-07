#include "mesh_provider.hpp"
#include "mesh.hpp"

MeshCreateParams MeshCreateParams::Default() {
    return MeshCreateParams();
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    return MeshCreateParams({ .meshVertexFormat = MeshVertexFormat::DefaultGui() });
}

RawMeshProvider::RawMeshProvider(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, const MeshCreateParams& params):
    vertices(vertices),
    indices(indices),
    MeshProvider(params)
{
}

std::pair<std::vector<float>, std::vector<unsigned int>> RawMeshProvider::GetMesh() const
{
    return std::make_pair(vertices, indices);
}

TextMeshProvider::TextMeshProvider(const MeshCreateParams& params, const std::shared_ptr<Material>& f) : MeshProvider(params), font(f)
{
}

unsigned int MeshVertexFormat::GetInstancedVertexSize() const {
    unsigned int size = 0;
    for (auto& atr : vertexAttributes) {
        if (atr.has_value() && atr->instanced) {
            size += atr->nFloats * sizeof(GLfloat);
        }
    }
    return size;
}

unsigned int MeshVertexFormat::GetNonInstancedVertexSize() const {
    unsigned int size = 0;
    for (auto& atr : vertexAttributes) {
        if (atr.has_value() && !atr->instanced) {
            size += atr->nFloats * sizeof(GLfloat);
        }
    }
    return size;
}

MeshVertexFormat::MeshVertexFormat(const MeshVertexFormat& other) : supportsAnimation(other.supportsAnimation), maxBones(other.maxBones), attributes(other.attributes) {
    // we have to calculate attribute offsets
    unsigned int noninstancedOffset = 0, instancedOffset = 0;
    for (auto& attrib : vertexAttributes) {
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

MeshVertexFormat::MeshVertexFormat(const MeshVertexFormat::FormatVertexAttributes& attrs, bool anims, unsigned int nBones) : supportsAnimation(anims), maxBones(nBones), attributes(attrs) {


    if (anims) {
        Assert(nBones == 4 || nBones == 16 || nBones == 64 || nBones == 256 || nBones == 1024 || nBones == 4096 || nBones == 16384);
    }

    // we have to calculate attribute offsets
    unsigned int noninstancedOffset = 0, instancedOffset = 0;
    for (auto& attrib : vertexAttributes) {
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

MeshVertexFormat MeshVertexFormat::DefaultTriplanarMapping(bool instancedColor)
{
    return MeshVertexFormat({
        .position = VertexAttribute {.nFloats = 3, .instanced = false},
        .textureUV = VertexAttribute {.nFloats = 2, .instanced = false},
        .textureZ = VertexAttribute {.nFloats = 3, .instanced = false},
        .color = VertexAttribute {.nFloats = 4, .instanced = instancedColor},
        .modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
        .normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true},
        .normal = VertexAttribute {.nFloats = 3, .instanced = false},
        .tangent = VertexAttribute {.nFloats = 3, .instanced = false},

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
                nAttributes = attribute->nFloats / 4;
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



MeshProvider::MeshProvider(const MeshCreateParams& p) : meshParams(p)
{
}
