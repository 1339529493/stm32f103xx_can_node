@REM .\build.bat app or .\build.bat bootloader

mkdir build
mkdir bin
cd build
del .\* /q /f /s
set CMAKE_CONFIG=-DTOOLCHAIN_PATH=C:/study_my/stm32/arm-none-eabi/bin -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -G "Ninja"

cmake .. %CMAKE_CONFIG% -DAPP_TAG=%1 && cmake --build . --target %1
cp %1.hex ..\bin
cp %1.elf ..\bin
cp %1.map ..\bin
cp %1.bin ..\bin