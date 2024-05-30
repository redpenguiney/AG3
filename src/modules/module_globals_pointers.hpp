class GraphicsEngine;
class ComponentRegistry;
class PhysicsEngine;
class SpatialAccelerationStructure;
class AudioEngine;
class MeshGlobals;
class GuiGlobals;
class ModuleTestInterface;

extern "C" {
    struct ModulesGlobalsPointers {
        GraphicsEngine* GE;
        MeshGlobals* MG;
        GuiGlobals* GG;
        ComponentRegistry* CR;
        PhysicsEngine* PE;
        SpatialAccelerationStructure* SAS;
        AudioEngine* AE;
        ModuleTestInterface* TEST;
    };
}