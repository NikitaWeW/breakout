# breakout
~~my first game.~~

~~Right now im setting up an environment for the development (ecs, renderer, scenes, parsers, implementing graphic stuff, etc.)~~

This is turning into a *mini(?)-*engine for some reason...

# building
uses cmake:

``` shell
cmake -S . -B build
cmake --build build
cmake --install build --prefix install
install/main
```

## tests

``` shell
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
cmake --install build --prefix install
install/tests
```

---

I try to keep the project cross-platform, but there are libraries like glfw that need to be built into a library for faster build time. Currently only **windows and linux** are supported. If you are using a different operating system, you will need to install and set the cmake `LIBRARIES` variable manually by adding `-DLIBRRAIES=\"all the necessary library files\"` to the cmake configure command.

# documentation

If the doxygen is found, the documentation will be automatically generated at build and be ready to be installed.
