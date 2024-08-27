//#pragma once
//#include <memory>
//
//class RenderComponent;
//class TransformComponent;
//
//template<typename T>
//class ModuleComponentHandleInteface {
//    public:
//    virtual ~ModuleComponentHandleInteface(); // apparently this is important idk why
//    virtual T* operator*();
//};
//
//class ModuleGameobjectInterface {
//    public:
//    virtual ~ModuleGameobjectInterface(); // apparently this is important idk why
//
//    // can't use std::string across dlls :(
//    virtual void SetName(const char* name) = 0;
//    virtual const char* GetName() = 0;
//
//    virtual ModuleComponentHandleInteface<TransformComponent> Transform() = 0;
//};
//
//class ModuleComponentRegistryInterface {
//    public:
//    virtual ~ModuleComponentRegistryInterface(); // apparently this is important idk why
//
//    static ModuleComponentRegistryInterface* Get();
//
//    virtual std::shared_ptr<ModuleGameobjectInterface> NewGameObject() = 0;
//};