# breakout
~~my first game.~~

Right now im setting up an environment for the development (ecs, renderer, scenes, parsers, implementing graphic stuff, etc.)

# building
uses cmake:

``` shell
cmake -S . -B build
cmake --build build
build/main
```

for debug build:
``` shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGENERATE_MSDF_FONTS=ON -DMSDF_ATLAS_GEN_PATH="path/to/msdf-atlas-gen/if/not/globally/available" -DCMAKE_CXX_FLAGS='-fdiagnostics-color=always -Wall' -G Ninja
```

I try to keep the project cross-platform, but there are libraries like glfw that need to be built into a library for faster build time. Currently only **windows and linux** are supported. If you are using a different operating system, you will need to install and set the cmake `LIBRARIES` variable manually by adding `-DLIBRRAIES=\"all the necessary library files\"` to the cmake configure command.