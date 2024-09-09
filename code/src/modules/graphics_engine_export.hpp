#pragma once
#include <memory>

class ShaderProgram;
class Material;
class Camera;
class Window;

class ModuleGraphicsEngineInterface {
    public:
    virtual ~ModuleGraphicsEngineInterface(); // apparently this is important idk why

    static ModuleGraphicsEngineInterface* Get();

    virtual void SetSkyboxShaderProgram(std::shared_ptr<ShaderProgram>) = 0;
    virtual void SetSkyboxMaterial(std::shared_ptr<Material>) = 0;

    virtual void SetPostProcessingShaderProgram(std::shared_ptr<ShaderProgram>) = 0;
    
    virtual void SetDefaultShaderProgram(std::shared_ptr<ShaderProgram>) = 0;
    virtual void SetDefaultGuiShaderProgram(std::shared_ptr<ShaderProgram>) = 0;
    virtual void SetDefaultBillboardGuiShaderProgram(std::shared_ptr<ShaderProgram>) = 0;

    virtual void SetDebugFreecamEnabled(bool) = 0;
    virtual void SetDebugFreecamPitch(double) = 0;
    virtual void SetDebugFreecamYaw(double) = 0;
    virtual void SetDebugFreecamAcceleration(double) = 0;

    virtual Camera& GetCurrentCamera() = 0;
    virtual Camera& GetMainCamera() = 0;
    virtual Camera& GetDebugFreecamCamera() = 0;

    virtual Window& GetWindow() = 0;

    virtual bool ShouldClose() = 0;
    
};