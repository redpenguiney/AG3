del "bin\\AG3.exe"
cd builddir
meson compile
cd ../
move "builddir\\AG3.exe" "./bin"
./run.bat