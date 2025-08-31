include(../cmake/cpm.cmake)

CPMAddPackage("gh:catchorg/Catch2@3.4.0")

file(GLOB_RECURSE TESTS_SOURCES "src/*.cpp")
file(GLOB_RECURSE TESTS_DEPENDENCIES_SOURCES "dependencies/compile/*")

add_executable(tests ${TESTS_SOURCES} ${TESTS_DEPENDENCIES_SOURCES})
target_include_directories(tests PRIVATE ../src ../dependencies/include src dependencies/include)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

install(TARGETS tests DESTINATION .)
