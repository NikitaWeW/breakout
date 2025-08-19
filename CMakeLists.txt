# This line will be changed if reconfiguration is required       
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

option(GENERATE_MSDF_FONTS OFF "generate msdf fonts using github.com/Chlumsky/msdf-atlas-gen")

project(breakout C CXX)

if(WIN32)
    file(GLOB_RECURSE LIBRARIES "dependencies/lib/windows/*")
elseif(UNIX AND NOT APPLE) # linux
    file(GLOB_RECURSE LIBRARIES "dependencies/lib/linux/*")
elseif(APPLE)
    message(WARNING "mac users will have to set LIBRARIES variable manualy via adding -DLIBRRAIES=\"all the necessary library files\" to cmake configure command.")
    # "Poor mac ysers" -- DEA__TH (Cosmic Horizons Dev) - 3/21/25, 5:30 PM
endif() 

if(GENERATE_MSDF_FONTS)
    if(NOT MSDF_ATLAS_GEN_PATH)
        set(MSDF_ATLAS_GEN_PATH msdf-atlas-gen)
    endif()
    if(NOT MSDF_MIN_GLYPH_SIZE)
        set(MSDF_MIN_GLYPH_SIZE 32)
    endif()
    set(MSDF_RESULT_FONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res/msdf-fonts")
    set(MSDF_CHARSET_FILEPATH "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/charset.txt")

    file(GLOB_RECURSE TTF_FONTS "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/*.ttf") # TODO: change the regular expression to recognize other supported types

    foreach(TTF_FONT_FILE IN LISTS TTF_FONTS)
        get_filename_component(RESULT_FILE_NAME "${TTF_FONT_FILE}" NAME_WE)
        execute_process(
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_QUIET ERROR_QUIET
            COMMAND ${MSDF_ATLAS_GEN_PATH} -font "${TTF_FONT_FILE}" -charset "${MSDF_CHARSET_FILEPATH}" -json "${MSDF_RESULT_FONTS_DIR}/${RESULT_FILE_NAME}.json" -imageout "${MSDF_RESULT_FONTS_DIR}/${RESULT_FILE_NAME}.png" -minsize "${MSDF_MIN_GLYPH_SIZE}"
        )
    endforeach()
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE DEPENDENCIES_SOURCES "dependencies/compile/*")

add_library(dependencies ${DEPENDENCIES_SOURCES})
target_include_directories(dependencies PRIVATE dependencies/include)

add_executable(main ${SOURCES})
target_link_libraries(main PRIVATE ${LIBRARIES} dependencies)
target_include_directories(main PRIVATE dependencies/include src)

file(GLOB_RECURSE GLM_PCH "dependencies/include/glm/*.hpp")
target_precompile_headers(main PRIVATE 
    src/core/ecs.hpp
    ${GLM_PCH}

)

if(BUILD_TESTS)
    Include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.9.1
    )
    FetchContent_MakeAvailable(Catch2)

    file(GLOB_RECURSE TESTS_SOURCES "tests/src/*.cpp")
    file(GLOB_RECURSE TESTS_DEPENDENCIES_SOURCES "tests/dependencies/compile/*")
    add_executable(tests ${TESTS_SOURCES} ${TESTS_DEPENDENCIES_SOURCES})
    target_include_directories(tests PRIVATE src dependencies/include tests/src tests/dependencies/include)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
endif()

install(DIRECTORY res DESTINATION .)
install(DIRECTORY shaders DESTINATION .)
install(TARGETS main DESTINATION .)