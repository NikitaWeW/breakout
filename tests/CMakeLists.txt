Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.1
)
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.15.0
)

if(NOT Catch2_POPULATED)
    message(STATUS "TESTS: Fetching Catch2...")
endif()
FetchContent_MakeAvailable(Catch2)
if(NOT EnTT_POPULATED)
    message(STATUS "TESTS: Fetching EnTT...")
endif()
FetchContent_MakeAvailable(EnTT)

file(GLOB_RECURSE TESTS_SOURCES "src/*.cpp")
file(GLOB_RECURSE TESTS_DEPENDENCIES_SOURCES "dependencies/compile/*")
add_executable(tests ${TESTS_SOURCES} ${TESTS_DEPENDENCIES_SOURCES})
target_include_directories(tests PRIVATE ../src ../dependencies/include src dependencies/include)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain EnTT::EnTT)

install(TARGETS tests DESTINATION .)
