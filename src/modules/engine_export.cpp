#include "engine_export.hpp"
#include "debug/log.hpp"
#include "modules/engine_export.hpp"

void Test::PrintTest() {
    DebugLogInfo("THEY ASKED UP TO PRINT TEST!!!");
}

Test::~Test() {

}

Test::Test() {

}

ModuleTestInterface* ModuleTestInterface::Get() {
    static Test t;
    return &t;
}