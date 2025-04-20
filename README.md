# breakout
my first game.

# building
uses cmake:
``` shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_CXX_FLAGS='-fdiagnostics-color=always -Wall' -G Ninja
cmake --build build
build/main
```