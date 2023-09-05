del bin/DEBUG.exe
g++ -c src/main.cpp -Wall -Wreturn-type -std=c++20
g++ -o bin/DEBUG.exe main.o -l64glfw3 -lopengl32 -lgdi32 -lglew64 -Wall -Wreturn-type -std=c++20 -g
del main.o
cd bin
start /B /wait gdb DEBUG.exe