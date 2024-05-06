cd modules
del test_module.o
del test_module.dll
g++ -c test_module.cpp -O3 -std=c++20
g++ -shared -o test_module.dll test_module.o
cd ../