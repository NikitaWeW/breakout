# breakout
my first game.

# building
uses cmake:

``` shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_CXX_FLAGS='-fdiagnostics-color=always -Wall' -G Ninja
cmake --build build
build/main
```

I try to keep the project cross-platform, but there are libraries like glfw that need to be built into a library for faster build time. Currently only **windows and linux** are supported. If you are using a different operating system, you will need to install and set the cmake `LIBRARIES` variable manually by adding `-DLIBRRAIES=\"all the necessary library files\"` to the cmake configure command.