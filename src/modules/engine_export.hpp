#pragma once

// In order to, with modules:
//    A: not have to recompile the module when i modify the engine
//    B: not have to compile the entire engine source code into every module
//    C: not have to worry about memory management/the fact that if an executable/module allocates memory, the same module has to delete it
//    D: not have to deal with multiple copies of static variables popping up and making me suffer
// Modules simply access these virtual interfaces to get all the functions/singletons they need. 
// todo: this does mean that all function calls to the engine will be virtual which could in theory hurt performance for situations where an engine function is called millions of times at once, can't imagine why you'd do that though

class ModuleTestInterface {
    public:
    virtual ~ModuleTestInterface(); // apparently this is important idk why

    virtual void PrintTest() = 0; 

    static ModuleTestInterface* Get();

};

class Test: public ModuleTestInterface {
    public:
    ~Test();
    Test();
    void PrintTest();
};

// class ModuleInterface {
//     public: 

//     virtual ~ModuleInterface();


// };