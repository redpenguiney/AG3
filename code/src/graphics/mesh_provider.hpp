#pragma once
#include <utility>
#include <memory>
#include <functional>
#include <vector>
#include <optional>
#include <debug/log.hpp>
#include <glm/vec3.hpp>
#include "texture_atlas.hpp"
#include <GL/glew.h>

class Material;

// Something a vertex has; color, position, normal, etc.
struct VertexAttribute {
    // the offset, in bytes, of the vertex attribute.
    // Calculated automatically, don't worry about it.
    unsigned short offset;

    // the number of floats in each vertex attribute. (position would be 3 in a 3D situation, for example).
    // Must be greater than 0 and <= 4, or == 9 (for a 3x3 matrix) or == 12 or 16 (for 4x3 and 4x4 matrices).
    unsigned short nFloats;

    // whether this vertex attribute is for every vertex, or for every renderComponent. (so you could either give each vertex a different color, or just store one color for each object)
    bool instanced;

    // if true, vertex attribute is integers insteadd of float
    bool integer = false;

    bool operator==(const VertexAttribute& other) const = default;
};

// Describes which vertex attributes a mesh has, which of them are instanced, and in what order they are in.
struct MeshVertexFormat {

    // should pretty much always be triangles unless ur tryna debug
    GLenum primitiveType = GL_TRIANGLES;

    // true if the mesh format supports animation.
    const unsigned int supportsAnimation;

    const unsigned int maxBones; // 0 if no animations; rounded up to nearest multiple of 4, meshes can have less than this.

    struct FormatVertexAttributes {
        std::optional<VertexAttribute> position;
        std::optional<VertexAttribute> textureUV; // TODO: when using colormap with fontmap, ability to use different uvs for each texture is needed? actually nvm i think just modifying shader and having 4-var uv is enough
        std::optional<VertexAttribute> textureZ; // texture z is seperate from the uvs because texture z is often instanced, but texture uv should never be instanced
        std::optional<VertexAttribute> color;
        std::optional<VertexAttribute> modelMatrix; // (will be all zeroes if not instanced, you must give it a value yourself which is TODO impossible) one 4x4 model matrix per thing being drawn, multiplying vertex positions by this puts them in the right position via rotating, scaling, and translating
        std::optional<VertexAttribute> normalMatrix; // (will be all zeroes if not instanced, you must give it a value yourself which is TODO impossible) normal matrix is like model matrix, but is 3x3 and for normals since it would be bad if a normal got scaled/translated  
        std::optional<VertexAttribute> normal; // lighting needs to know normals
        std::optional<VertexAttribute> tangent; // atangent vector is perpendicular to the normal and lies along the face of a triangle, used for normal/parallax mapping
        std::optional<VertexAttribute> arbitrary1;
        std::optional<VertexAttribute> arbitrary2;
    };


    // if animation support is requested, nBones must be a power of 4 greater than 1.
    MeshVertexFormat(const FormatVertexAttributes&, bool anims = false, unsigned int nBones = 0);
    MeshVertexFormat(const MeshVertexFormat&); // copy constructor has to recalculate offsets

    const static inline unsigned int N_ATTRIBUTES = 10;

    union { // this cursed union lets us either refer to vertex attributes by name, or iterate through them using the member vertexAttributes[]
        FormatVertexAttributes attributes; // BRUH I HAD TO NAME THE STRUCT MID C++ IS MID

        std::optional<VertexAttribute> vertexAttributes[N_ATTRIBUTES];
    };

    // TODO: might be slow
    static unsigned int AttributeIndexFromAttributeName(unsigned int name) {
        switch (name) {
        case POS_ATTRIBUTE_NAME:
            return 0;
        case TEXTURE_UV_ATTRIBUTE_NAME:
            return 1;
        case TEXTURE_Z_ATTRIBUTE_NAME:
            return 2;
        case COLOR_ATTRIBUTE_NAME:
            return 3;
        case MODEL_MATRIX_ATTRIBUTE_NAME:
            return 4;
        case NORMAL_MATRIX_ATTRIBUTE_NAME:
            return 5;
        case NORMAL_ATTRIBUTE_NAME:
            return 6;
        case TANGENT_ATTRIBUTE_NAME:
            return 7;
        case ARBITRARY_ATTRIBUTE_1_NAME:
            return 8;
        case ARBITRARY_ATTRIBUTE_2_NAME:
            return 9;
        default:
            DebugLogError("YOU FOOL, ", name, " IS NO VALID ATTRIBUTE NAME");
            abort();
        }
    }

    // TODO: AUTOMATICALLY ASSIGN THESE NAMES BOTH IN SHADER AND OUTSIDE

    static inline const unsigned int POS_ATTRIBUTE_NAME = 0;
    static inline const unsigned int COLOR_ATTRIBUTE_NAME = 1;
    static inline const unsigned int TEXTURE_UV_ATTRIBUTE_NAME = 2;
    static inline const unsigned int TEXTURE_Z_ATTRIBUTE_NAME = 3;
    static inline const unsigned int NORMAL_ATTRIBUTE_NAME = 4;
    static inline const unsigned int TANGENT_ATTRIBUTE_NAME = 5;
    static inline const unsigned int MODEL_MATRIX_ATTRIBUTE_NAME = 6;
    /// model matrix takes up slots 7, 8, and 9 too because only one vec4 per attribute
    static inline const unsigned int NORMAL_MATRIX_ATTRIBUTE_NAME = 10;
    // normal matrix takes up slots 11 and 12 too for the same reason
    static inline const unsigned int ARBITRARY_ATTRIBUTE_1_NAME = 13;
    static inline const unsigned int ARBITRARY_ATTRIBUTE_2_NAME = 14;

    // returns combined size in bytes of each non-instanced vertex attribute for one vertex
    unsigned int GetNonInstancedVertexSize() const;

    // returns combined size in bytes of each instanced vertex attribute for one vertex
    unsigned int GetInstancedVertexSize() const;

    // Returns a simple mesh vertex format that should work for normal people doing normal things in 3D.
    // nBones should equal 0 if you don't want animations, otherwise MUST be multiple of 4.
    // noninstanced (XYZ, TextureXY, NormalXYZ, TangentXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ). TODO make actually accurate
    // instanced: model matrix, normal matrix, rgba if instanced, textureZ if instanced
    static MeshVertexFormat Default(unsigned int nBones = 0, bool instancedColor = true, bool instancedTextureZ = true);

    // Returns a simple mesh vertex format that should work for normal people doing normal things with GUI. Just XYZ UV (plus instanced stuff).
    static MeshVertexFormat DefaultGui();

    // triplanar mapping is a shader technique that allows for texturing meshes that don't have UVs, which is useful for things like procedural terrain.
    // Has to be used with a buffer that corresponds vertex indices to textureZ/atlas data.
    // noninstanced: XYZ, NormalXYZ, RGBA if !instanceColor.
    // instanced:  model matrix, normal matrix, rgba if instanced
    static MeshVertexFormat DefaultTriplanarMapping(bool instancedColor = true);

    // Takes a VAO and sets its noninstanced vertex attributes using VertexAttribPointer().
    // The VAO must ALREADY BE BOUND.s
    void SetNonInstancedVaoVertexAttributes(unsigned int& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const;

    // Takes a VAO and sets its instanced vertex attributes using VertexAttribPointer().
    // The VAO must ALREADY BE BOUND.
    void SetInstancedVaoVertexAttributes(unsigned int& vaoId, unsigned int instancedSize, unsigned int nonInstancedSize) const;
    void HandleAttribute(unsigned int& vaoId, const std::optional<VertexAttribute>& attribute, const unsigned int attributeName, bool justInstanced, unsigned int instancedSize, unsigned int nonInstancedSize) const;

    bool operator==(const MeshVertexFormat& other) const;
};

struct MeshCreateParams {
	// if nullopt, vertex format will be determined automatically
	std::optional<MeshVertexFormat> meshVertexFormat = std::nullopt; // todo: unconst?

	// default value of textureZ for the mesh's vertices, if that is a noninstanced vertex attribute
	float textureZ = -1.0;

	// default value of transaprency/alpha for the mesh's vertices, if color is a noninstanced 4-component vertex attribute
	float opacity = 1.0;

	// meshpool will make room for this many objects to use this mesh (if you go over it's fine, but performance may be affected)
	// the memory cost of this is ~64 * expectedCount bytes if you ask for space you don't nee
	// default of 1024 should be fine in almost all cases
	// NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or TODO memory issues i should probably address at somepoint
	unsigned int expectedCount = 1024;

	// size should always be normalized for collisions/physics to work. Only set to false if you're making a weird mesh like the skybox or gui or something.
	// to actually change a rendercomponent's mesh's size, scale its transform component, using Mesh::originalSize if you want the mesh at its correct size.
	bool normalizeSize = true;

	static MeshCreateParams Default();
	static MeshCreateParams DefaultGui();
};

// When you create a Mesh, it needs to get its vertices and indices from somewhere.
// When loading from files via assimp, you use Mesh::MultiFromFile().
	// This is because assimp decides basically everything about the meshes (yes, plural which is why assimp file loading doesn't use MeshProvider).
// For other stuff, you use Mesh::New() and provide a object that inherits from MeshProvider.
class MeshProvider {
public:
    MeshProvider(const MeshCreateParams&);
	MeshCreateParams meshParams = MeshCreateParams::Default();

	// returns the vertices and indices.
	virtual std::pair < std::vector < float > , std::vector< unsigned int> > GetMesh() const = 0;

	virtual ~MeshProvider() = default;
};

// Mesh provider that takes user-specified vertices and indices.
// verts must be organized in accordance with the given meshVertexFormat.
class RawMeshProvider: public MeshProvider {
public:
	RawMeshProvider(const std::vector<float>& vertices = {}, const std::vector<unsigned int>& indices = {}, const MeshCreateParams& params = MeshCreateParams::Default());

    std::pair < std::vector < float >, std::vector< unsigned int> > GetMesh() const override;

	std::vector<float> vertices;
	std::vector<unsigned int> indices;
};

enum class HorizontalAlignMode {
	Left,
	Center,
	Right
};

enum class VerticalAlignMode {
	Top,
	Center,
	Bottom
};

struct TextMeshCreateParams {
	float leftMargin = -1000, rightMargin = 1000, topMargin = 0, bottomMargin = 0; // in pixels, 0 is the center of the ui text is being put on. top and bottom margin are only respected for top and bottom vertical alignment respectively.
	float lineHeightMultiplier = 1; // 1 is single spaced, 2 is double spaced, etc.
	HorizontalAlignMode horizontalAlignMode = HorizontalAlignMode::Center;
	VerticalAlignMode verticalAlignMode = VerticalAlignMode::Center;
	bool wrapText = true;
};

// Mesh provider that creates a mesh for text.
class TextMeshProvider: public MeshProvider {
public:
    TextMeshProvider(const MeshCreateParams& = MeshCreateParams::Default(), const std::shared_ptr<Material>& font = nullptr);

    std::pair < std::vector < float >, std::vector< unsigned int> > GetMesh() const override;

    std::string text = "Placeholder text.";

    // must be valid ptr to material with fontmap
    const std::shared_ptr<Material> font = nullptr;
	TextMeshCreateParams textParams;
};

// Mesh provider that uses dual contouring to generate a mesh based from a signed distance function representing a terrain surface.
// Created mesh samples points between p1 and p2 with the given resolution. 
// The distance function must be continuous, and return distance values between -1 and 1 (TODO CONFIRM)
// If a texture atlas is provided, it will use it for UVs and/or vertex colors if the format supports them.
// Will also generate normals, tangents, etc. if requested by the format.
// Mesh will be designed to be of dimensions size.
// Note that this provider may return an empty mesh (if the terrain area sampled is completely solid or completely hollow). TODO is that okay?
// TODO currently basically broken
class DualContouringMeshProvider: public MeshProvider {
public:
    DualContouringMeshProvider(const MeshCreateParams& = MeshCreateParams::Default());

    // defined in mesh_voxels.cpp
    std::pair < std::vector < float >, std::vector< unsigned int> > GetMesh() const override;

    glm::vec3 point1;
    glm::vec3 point2;
    float resolution;
    std::function<float(glm::vec3)> distanceFunction;
    std::optional<const TextureAtlas*> atlas = std::nullopt; // sorry its a pointer, should be a reference but it doesn't like optionals with references.
    bool fixVertexCenters = false; // if true will create minecraft-style blocky terrain
};

// Mesh provider that uses marching cubes based on a signed distance function.
class MarchingCubesMeshProvider : public MeshProvider {
public:
    MarchingCubesMeshProvider(const MeshCreateParams & = MeshCreateParams::Default());

    // defined in mesh_voxels.cpp
    std::pair < std::vector < float >, std::vector< unsigned int> > GetMesh() const override;

    glm::vec3 point1;
    glm::vec3 point2;
    float resolution;
    std::function<float(glm::vec3)> distanceFunction;
    std::optional<const TextureAtlas*> atlas = std::nullopt; // sorry its a pointer, should be a reference but it doesn't like optionals with references.
    bool fixVertexCenters = false; // if true will create minecraft-style blocky terrain
};