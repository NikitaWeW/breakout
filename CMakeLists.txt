cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(breakout C CXX)

######################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/cpm.cmake)

# Setup assimp.
# WARNING: Remove when adding exporting. Remove when mature.
set(ASSIMP_NO_EXPORT ON)

# WARNING: Not all loaders are built. Perhaps remove this when mature.
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
foreach(LOADER FBX OBJ GLTF)
    set(ASSIMP_BUILD_${LOADER}_IMPORTER ON)
endforeach()

set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)

# nicecs is still in parallel development (minor tweaks mostly), using commit hashes to avoid spamming releases. Change to release version when mature.
CPMAddPackage("gh:nikitawew/nicecs#c2b2f0f")
CPMAddPackage("gh:nlohmann/json@3.12.0")
CPMAddPackage("gh:pboettch/json-schema-validator#2.4.0")
CPMAddPackage("gh:glfw/glfw#3.4")
CPMAddPackage("gh:gabime/spdlog@1.16.0")
CPMAddPackage("gh:assimp/assimp@6.0.2")
CPMAddPackage("gh:g-truc/glm#1.0.2")

file(GLOB_RECURSE DEPENDENCIES_SOURCES "dependencies/compile/*")
add_library(dependencies ${DEPENDENCIES_SOURCES})
target_include_directories(dependencies PRIVATE dependencies/include)
target_include_directories(dependencies INTERFACE dependencies/include)
target_link_libraries(dependencies INTERFACE glfw ecs spdlog assimp glm nlohmann_json::nlohmann_json nlohmann_json_schema_validator::validator)

######################################################################

file(GLOB_RECURSE ENGINE_SOURCES "src/engine/*.cpp")

add_library(engine ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC dependencies)
target_include_directories(engine PUBLIC include)
target_compile_definitions(engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_precompile_headers(engine PRIVATE 
    include/engine/Engine.hpp
)

install(DIRECTORY DESTINATION engine)
install(DIRECTORY include DESTINATION engine/include)
install(TARGETS engine DESTINATION engine/lib)

######################################################################

file(GLOB_RECURSE SOURCES "src/app/*.cpp")

CPMAddPackage("gh:CLIUtils/CLI11@2.6.1")

add_executable(app ${SOURCES})
target_include_directories(app PRIVATE src src/app)
target_link_libraries(app PRIVATE engine CLI11::CLI11)

install(DIRECTORY DESTINATION app)
install(DIRECTORY res DESTINATION app)
install(DIRECTORY shaders DESTINATION app)
install(TARGETS app DESTINATION app)

######################################################################
