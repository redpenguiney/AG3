@REM del "bin\\AG3.exe"
@REM cd build
@REM meson compile
@REM cd ../
@REM move "build\\AG3.exe" "./bin"
cd bin
gdb AG3.exe
cd ../