g++ -c modules/test_module.cpp
move "test_module.o" "modules"
g++ -shared -o modules/test_module.dll modules/test_module.o