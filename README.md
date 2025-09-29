# breakout
~~my first game.~~

~~Right now im setting up an environment for the development (ecs, renderer, scenes, parsers, implementing graphic stuff, etc.)~~

This is turning into a *mini(?)-*engine for some reason...

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