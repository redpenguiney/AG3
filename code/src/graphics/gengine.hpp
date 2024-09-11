#pragma once
#include "mesh.hpp"
#include "meshpool.hpp"
#include "modules/graphics_engine_export.hpp"
#include "window.hpp"
#include "shader_program.hpp"
#include "debug/assert.hpp"
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../debug/debug.hpp"
#include "camera.hpp"
#include "../gameobjects/transform_component.hpp"
#include "../utility/utility.hpp"
#include "material.hpp"
#include "renderable_mesh.hpp"
#include "framebuffer.hpp"
// #include "gameobjects/render_component.hpp"

// struct MeshLocation {
//     unsigned int poolId; // uuid of the meshpool 
//     unsigned int poolSlot;
//     unsigned int poolInstance;
//     unsigned int materialId;
//     unsigned int shaderProgramId;
//     bool initialized;
//     MeshLocation() {
//         initialized = false;
//     }
// };

class RenderComponent;
class RenderComponentNoFO;

// Handles graphics, obviously.
class GraphicsEngine: public ModuleGraphicsEngineInterface {
public:
    // incremented by one every time RenderScene() is called.
    long long frameId = 0;

    // the shader used to render the skybox. Can freely change this with no issues.
    std::shared_ptr<ShaderProgram> skyboxShaderProgram;

    // skybox material. Must be a cubemap.
    std::shared_ptr<Material> skyboxMaterial;

    // Which layer of the skyboxMaterial we should actually be using.
    unsigned int skyboxMaterialLayer; 

    // Postprocessing shader, aka the shader used to render the screen quad that the actual world was rendered onto the texture of.
    // Can freely change this with no issues.
    std::shared_ptr<ShaderProgram> postProcessingShaderProgram; 

    // Default shader program used for new RenderComponents. You can freely change this with no issues (I think??? TODO what happens).
    std::shared_ptr<ShaderProgram> defaultShaderProgram;

    // Default shader program used when creating instances of the Gui class. Should be able to freely change this with no issues (TODO verify)
    std::shared_ptr<ShaderProgram> defaultGuiShaderProgram;

    // Same as defaultGuiShaderProgram, but for billboard uis. Should be able to freely change this with no issues (TODO verify)
    std::shared_ptr<ShaderProgram> defaultBillboardGuiShaderProgram;

    // draw wireframe instead of triangles for debugging
    void SetWireframeEnabled(bool);

    // freecam is just a thing for debugging
    bool debugFreecamEnabled = false;
    

    // Returns main camera if debug freecam is disabled, otherwise returns debug freecam camera.
    Camera& GetCurrentCamera() override;
    Camera& GetMainCamera() override;
    Camera& GetDebugFreecamCamera() override;

    void SetDebugFreecamEnabled(bool) override;
    void SetDebugFreecamPitch(double) override;
    void SetDebugFreecamYaw(double) override;
    void SetDebugFreecamAcceleration(double) override;

    double debugFreecamPitch = 0; // in degrees, don't get tripped up when you do lookvector which wants radians
    double debugFreecamYaw = 0;

    void SetSkyboxShaderProgram(std::shared_ptr<ShaderProgram>) override;
    void SetSkyboxMaterial(std::shared_ptr<Material>) override;

    void SetPostProcessingShaderProgram(std::shared_ptr<ShaderProgram>) override;
    
    void SetDefaultShaderProgram(std::shared_ptr<ShaderProgram>) override;
    void SetDefaultGuiShaderProgram(std::shared_ptr<ShaderProgram>) override;
    void SetDefaultBillboardGuiShaderProgram(std::shared_ptr<ShaderProgram>) override;

    Window& GetWindow() override;

    // Be advised: If you're tryna get camera position/orientation regardless of whether debugfreecam is enabled, use the GetCurrentCamera() method instead.
    Camera camera;
    
    Window window = Window(720, 720); // handles windowing, interfaces with GLFW in general

    static GraphicsEngine& Get();

    // Used by Texture to throw an error if someone tries to unload a texture being used
    //bool IsMaterialInUse(unsigned int textureId);

    // Used by ShaderProgram to throw an error if someone tries to unload a shader being used.
    //bool IsShaderProgramInUse(unsigned int shaderId);

    // returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
    bool ShouldClose();

    // Draws everything. dt should be time since this function was last called.
    void RenderScene(float dt);

    // Used by mesh loaders when they fail to load a material from a file. (Not used by material/texture constructors, those throw an exception if they fail to load a file.)
    // You can probably change this to whatever you want. (TODO confirm)
    std::shared_ptr<Material> errorMaterial; 
    float errorMaterialTextureZ; 
    // std::shared_ptr<Material> GetDefaultMaterial(); // TODO

    // Render components that use a dynamic mesh add themselves to this unordered map (and remove themselves on destruction).
    // Key is mesh id, value is ptr to rendercomponent which uses that mesh.
    // Needed so that mesh.cpp can update the draw handles of render components if, in order to modify a dynamic mesh, the mesh has to be moved to a different location in its meshpool.
    std::unordered_map<unsigned int, std::vector<RenderComponent*>> dynamicMeshUsers;

    // key is meshId, value is <meshpool index, mesh slot> for the meshpool which contains this dynamic mesh.
    std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>> dynamicMeshLocations;

    // used by various debug features, can probably change freely with no isses (TODO check) 
    std::shared_ptr<ShaderProgram> crummyDebugShader;

    // float is dt
    Event<float> preRenderEvent;
    // float is dt; note, fires before buffers are flipped which might be bad idk lol
    Event<float> postRenderEvent;

private:

    // thing we send to gpu to tell it about a light
    struct PointLightInfo {
        glm::vec4 colorAndRange; // w-coord is range, xyz is rgb
        glm::vec4 relPos; // w-coord is padding; openGL wants everything on a vec4 alignment
    };

    struct SpotLightInfo {
        glm::vec4 colorAndRange; // w-coord is range, xyz is rgb
        glm::vec4 relPosAndInnerAngle; // w-coord is cos(inner angle); openGL wants everything on a vec4 alignment
        glm::vec4 directionAndOuterAngle; // w-coord is cos(outer angle)
    };

    unsigned int pointLightCount; // updated every frame by UpdateLights()
    unsigned int spotLightCount; // updated every frame by UpdateLights()

    BufferedBuffer pointLightDataBuffer;
    BufferedBuffer spotLightDataBuffer;

    bool wireframeDrawing = false;

    Camera debugFreecamCamera;
    double debugFreecamSpeed = 0;
    double debugFreecamAcceleration = 0.01;

    friend class Mesh; // literally just friend so dynamic mesh support is less work for me idc about keeping it modular
    friend class RenderComponent;
    friend class Meshpool;
    // friend class RenderComponentNoFO;
    
    // SSBO that stores all points lights so that the GPU can use them.
    // Format is:
    // struct lightInfo {
        // vec4 colorAndRange; // w-coord is range, xyz is rgb
        // vec4 rel_pos; // w-coord is padding
    // }
    // unsigned int numLights;
    // vec3 padding
    // lightInfo pointLights[];

    
    
    

    RenderableMesh* skybox; 

    // everything is drawn onto this framebuffer, and then this framebuffer's texture is used to draw a screen quad on the default framebuffer with a post processing shader.
    // todo: make it not do post processing when this isn't here
    std::optional<Framebuffer> mainFramebuffer;

    // Used for postprocessing, the mesh is just a quad that covers the screen, nothing deep.
    RenderableMesh screenQuad;

    // just a little thing to visualize axis
    void DebugAxis();

    // meshpools are highly optimized objects used for very fast drawing of meshes
    // for technical reasons, we need a different meshpool for each vertex format (TODO: technically not true, but there's zero benefit to splitting it so whateves)
    // Key is meshpool id.
    // materialId can also == 0 for no material.
    // some may be nullptr.
    std::vector<Meshpool*> meshpools;
    IdProvider meshpoolIdProvider;

    // Cache of meshes to add when addCachedMeshes is called. 
    // Used so that instead of adding 1 mesh to a meshpool 1000 times, we just add 1000 instances of a mesh to meshpool once to make creating renderable objects faster.
    // Keys are meshId, then shader id, then material id.
    // IMPORTANT TODO: nothing prevents material/shader deletion of the things with these ids.
    std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::vector<RenderComponent*>>>> renderComponentsToAdd;

    // Keeps track of rendercomponents whose instanced vertex attributes (color, textureZ, etc.) need to be updated (since due to multiple buffering it has be updated over multiple frames).
    template <typename AttributeType> 
    class InstancedVertexAttributeUpdater {
    public:
        /*static_assert(
            std::is_same_v<AttributeType, float>() ||
            std::is_same_v<AttributeType, glm::vec2>() ||
            std::is_same_v<AttributeType, glm::vec3>() || 
            std::is_same_v<AttributeType, glm::vec4>() || 
            std::is_same_v<AttributeType, glm::mat3x3>() ||
            std::is_same_v<AttributeType, glm::mat4x4>()
        );*/

        void AddUpdate(RenderComponent* comp, unsigned int attributeName, const AttributeType& newValue) {
            updates.push_back(AttributeUpdate{
                .renderComp = comp,
                .newValue = newValue,
                .attributeIndex = MeshVertexFormat::AttributeIndexFromAttributeName(attributeName),
                .updatesRemaining = INSTANCED_VERTEX_BUFFERING_FACTOR
            });
        }

        // exists because if someone creates an object, lets it exist for one frame, then deletes it, we'll have hanging references (RenderComponent destructor calls this)
        // that shouldn't happen very much so perf. not too important
        void CancelUpdate(RenderComponent* comp) {
            canceledUpdates.push_back(comp);
        }

        void ApplyUpdates() {
            unsigned int i = 0;

            
            std::vector<unsigned int> indicesToRemove;
            for (auto& update : updates) {

                // TODO: optimizations to be made here
                for (auto it = canceledUpdates.begin(); it != canceledUpdates.end(); it++) {
                    if (*it == update.renderComp) {
                        it = canceledUpdates.erase(it);
                    }
                }

                if (update.renderComp->meshpoolId != -1) { // we can't set these values until the render component gets a mesh pool
                    GraphicsEngine::Get().meshpools[update.renderComp->meshpoolId]->SetInstancedVertexAttribute<AttributeType>(update.renderComp->drawHandle, update.attributeIndex, update.newValue);
                    update.updatesRemaining -= 1;
                    if (update.updatesRemaining == 0) {
                        indicesToRemove.push_back(i);
                    }
                    i++;
                }


            }

            for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
                updates[*it] = updates.back();
                updates.pop_back();
            }
            indicesToRemove.clear();
        }
    private:
        struct AttributeUpdate{
            RenderComponent* renderComp; // fortunately, it doesn't actually matter if this pointer is to a destroyed component
            AttributeType newValue;
            unsigned int attributeIndex;

            unsigned int updatesRemaining = INSTANCED_VERTEX_BUFFERING_FACTOR;
        };

        std::vector<AttributeUpdate> updates;
        std::vector<RenderComponent*> canceledUpdates;
    };

    InstancedVertexAttributeUpdater<float> updater1;
    InstancedVertexAttributeUpdater<glm::vec2> updater2;
    InstancedVertexAttributeUpdater<glm::vec3> updater3;
    InstancedVertexAttributeUpdater<glm::vec4> updater4;
    InstancedVertexAttributeUpdater<glm::mat3x3> updater3x3;
    InstancedVertexAttributeUpdater<glm::mat4x4> updater4x4;

    GraphicsEngine();
    ~GraphicsEngine();

    void UpdateMainFramebuffer();
    void CalculateLightingClusters();
    void UpdateLights();
    void DrawSkybox();
    void Update();
    void UpdateRenderComponents(float dt);

    // updates the freecam based off user input (WASD and mouse)
    void UpdateDebugFreecam();
    void AddCachedMeshes();

    // Commits changes to vertex/instance data so that it's synced with the GPU.
    void CommitMeshpools();

    void FlipMeshpoolBuffers();
    // postProc is true if what's being drawn SHOULD do post proc
    void DrawWorld(bool postProc);

    // void SetColor(const RenderComponent& component, const glm::vec4& rgba);
    void SetModelMatrix(const RenderComponent& component, const glm::mat4x4& model);
    void SetNormalMatrix(const RenderComponent& component, const glm::mat3x3& normal);
    void SetBoneState(const RenderComponent& component, unsigned int nBones, glm::mat4x4* boneTransforms);

    // void SetArbitrary1(const RenderComponent& component, const float arb);

    // set to -1.0 for no texture
    // void SetTextureZ(const RenderComponent& component, const float textureZ);

    // Returns a drawId used to modify the mesh later on.
    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called.
    // Contents of MeshLocation are undefined until addCachedMeshes() is called, except for textureId, shaderProgramId, and initialized.
    // Before addChachedMeshes is called, meshLocation->initialized == false and after == true
    void AddObject(unsigned int shaderId, unsigned int materialId, unsigned int meshId, RenderComponent* component);
};

