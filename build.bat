del bin/AG3.exe
g++ -c src/main.cpp -Wall -Wreturn-type -std=c++20
g++ -o bin/AG3.exe main.o -l64glfw3 -lopengl32 -lgdi32 -lglew64 -Wall -Wreturn-type -std=c++20
start /B /wait bin/AG3.exe
del main.o