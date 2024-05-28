del "bin\\AG3.exe"
cd build
meson compile
cd ../
move "build\\libtest_module.dll" "./modules"
move "build\\AG3.exe" "./bin"
move "build\\libtest_module.dll" "./modules"