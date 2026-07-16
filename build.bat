mkdir build
cd build
del .\* /q /f /s
set CMAKE_CONFIG=-DTOOLCHAIN_PATH=C:/study_my/stm32/arm-none-eabi/bin -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -G "Ninja"
cmake .. %CMAKE_CONFIG% && cmake --build . --target can_c8