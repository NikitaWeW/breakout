# This line will be changed if reconfiguration is required  
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

project(breakout C CXX)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE DEPENDENCIES_SOURCES "dependencies/compile/*")
add_executable(main ${SOURCES} ${DEPENDENCIES_SOURCES})

if(WIN32)
    file(GLOB_RECURSE LIBRARIES "dependencies/lib/windows/*")
elseif(UNIX AND NOT APPLE) # linux
    file(GLOB_RECURSE LIBRARIES "dependencies/lib/linux/*")
elseif(APPLE)
    message(WARNING "mac users will have to set LIBRARIES variable manualy via adding -DLIBRRAIES=\"all the necessary library files\" to cmake configure command.")
    # "Poor mac ysers" -- DEA__TH (Cosmic Horizons Dev) - 3/21/25, 5:30 PM
endif() 

target_link_libraries(main PRIVATE ${LIBRARIES})
target_include_directories(main PRIVATE dependencies/include src)

install(DIRECTORY res DESTINATION res)
install(TARGETS main DESTINATION .)