// Unfortunately, one cannot just have a static variable or normal global variables because 
    // we use dlls/shared libraries, and each library would get its own copy of the variable.
// Thus, all globals/singleton classes will inherit from the SharedGlobal class, which resolves this problem.
    // When this class is compiled for the main executable, it uses a normal static variable.
    // When this class is compiled for dlls, it uses a static pointer to the actual global.
        // The executable will then supply the neccesary pointer using the SetModulePointer() method.

class SharedGlobal {
public:
    

private:
};