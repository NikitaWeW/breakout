cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(breakout C CXX)

######################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/cpm.cmake)

set(ASSIMP_WARNINGS_AS_ERRORS OFF)
CPMAddPackage("gh:glfw/glfw#3.4")
CPMAddPackage("gh:nikitawew/nicecs@1.5.3")
CPMAddPackage("gh:gabime/spdlog@1.16.0")
CPMAddPackage("gh:assimp/assimp@6.0.2")
CPMAddPackage("gh:g-truc/glm#1.0.2")

file(GLOB_RECURSE DEPENDENCIES_SOURCES "dependencies/compile/*")
add_library(dependencies ${DEPENDENCIES_SOURCES})
target_include_directories(dependencies PRIVATE dependencies/include)
target_include_directories(dependencies INTERFACE dependencies/include)
target_link_libraries(dependencies INTERFACE glfw ecs spdlog assimp glm)

######################################################################

file(GLOB_RECURSE ENGINE_SOURCES "src/engine/*.cpp")

add_library(engine ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC dependencies)
target_include_directories(engine PRIVATE src)
target_compile_definitions(engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_precompile_headers(engine PRIVATE 
    src/engine/engine.hpp
)

install(DIRECTORY DESTINATION engine)
install(DIRECTORY DESTINATION engine/lib)
install(DIRECTORY DESTINATION engine/include)
install(DIRECTORY src/engine DESTINATION engine/include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(TARGETS engine DESTINATION engine/lib)

######################################################################

file(GLOB_RECURSE SOURCES "src/app/*.cpp")

add_executable(app ${SOURCES})
target_include_directories(app PRIVATE src)
target_link_libraries(app PRIVATE engine)

install(DIRECTORY DESTINATION app)
install(DIRECTORY res DESTINATION app)
install(DIRECTORY shaders DESTINATION app)
install(TARGETS app DESTINATION app)

######################################################################
