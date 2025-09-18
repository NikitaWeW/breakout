cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(breakout C CXX)

######################################################################

include(cmake/cpm.cmake)

CPMAddPackage("gh:glfw/glfw#3.4")
CPMAddPackage("gh:nikitawew/ecs@1.3.0")

file(GLOB_RECURSE DEPENDENCIES_SOURCES "dependencies/compile/*")
add_library(dependencies ${DEPENDENCIES_SOURCES})
target_include_directories(dependencies PUBLIC dependencies/include)

######################################################################

file(GLOB_RECURSE ENGINE_SOURCES "src/engine/*.cpp")

add_library(engine ${ENGINE_SOURCES})
target_link_libraries(engine PUBLIC ecs glfw dependencies)
target_include_directories(engine PRIVATE src)

######################################################################

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES ${ENGINE_SOURCES})

add_executable(main ${SOURCES})
target_include_directories(main PRIVATE src)
target_link_libraries(main PRIVATE engine)

target_precompile_headers(main PRIVATE 
    src/engine/config.hpp # to configure ecs.hpp
    ${ECS_SOURCE_DIR}/ecs.hpp
    dependencies/include/glm/glm.hpp
    dependencies/include/glm/gtc/quaternion.hpp
    dependencies/include/glm/gtc/matrix_transform.hpp
)

install(DIRECTORY res DESTINATION .)
install(DIRECTORY shaders DESTINATION .)
install(TARGETS main DESTINATION .)

######################################################################

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
