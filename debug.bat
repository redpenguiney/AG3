del "bin\\AG3.exe"
cd build
meson compile
cd ../
move "build\\AG3.exe" "./bin"
cd bin
gdb AG3.exe
cd ../