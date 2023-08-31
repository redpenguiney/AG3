del bin/AG3.exe
g++ -c src/main.cpp -Wall -Wreturn-type -O1 -std=c++20
g++ -o bin/AG3.exe main.o -L:./ -lglfw3, -lopengl32 -lglew64 -Wall -Wreturn-type -std=c++20
bin/AG3.exe