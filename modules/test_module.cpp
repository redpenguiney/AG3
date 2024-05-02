#include <iostream>

extern "C" {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
__declspec (dllexport) void OnInit() {
    std::cout << "OH YEAH LETS GO\n";
}
#endif

}
