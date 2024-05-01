#include <iostream>

extern "C" {

#if defined(__WIN32) || defined(__WIN64)
__declspec (dllexport) void OnInit() {
    std::cout << "OH YEAH LETS GO\n";
}
#endif

}
