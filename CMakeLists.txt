cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(breakout C CXX)

######################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/cpm.cmake)
if(NOT CPM_USE_LOCAL_PACKAGES)
    set(CPM_USE_LOCAL_PACKAGES, ON)
endif()

CPMAddPackage("gh:glfw/glfw#3.4")
CPMAddPackage("gh:nikitawew/nicecs@1.5.2")
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

target_precompile_headers(engine PRIVATE 
    src/engine/engine.hpp
)

######################################################################

file(GLOB_RECURSE SOURCES "src/app/*.cpp")

add_executable(main ${SOURCES})
target_include_directories(main PRIVATE src)
target_link_libraries(main PRIVATE engine)

find_package(Gperftools)
if(GPERFTOOLS_FOUND)
    target_link_libraries(main PRIVATE ${GPERFTOOLS_LIBRARIES})
endif()
set_property(TARGET main APPEND_STRING PROPERTY LINK_OPTIONS "-lprofiler")

install(DIRECTORY res DESTINATION .)
install(DIRECTORY shaders DESTINATION .)
install(TARGETS main DESTINATION .)

######################################################################

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
