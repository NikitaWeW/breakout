# This line will be changed if reconfiguration is required
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(breakout C CXX)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE DEPENDENCIES_SOURCES "dependencies/compile/*")

if(WIN32)
    file(GLOB_RECURSE LIBRARIES "dependencies/lib/windows/*")
elseif(UNIX AND NOT APPLE) # linux
    file(GLOB_RECURSE LIBRARIES "dependencies/lib/linux/*")
else()
    find_package(glfw3 REQUIRED)

    target_link_libraries(main PRIVATE ${GLFW_LIBRARIES})
endif()

add_executable(main ${SOURCES} ${DEPENDENCIES_SOURCES})

target_link_libraries(main PRIVATE ${LIBRARIES})
target_include_directories(main PRIVATE dependencies/include src)

install(DIRECTORY res DESTINATION res)
install(TARGETS main DESTINATION .)