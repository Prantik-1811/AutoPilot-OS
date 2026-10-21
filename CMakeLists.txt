cmake_minimum_required(VERSION 3.20)
project(AutoPilotOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

add_library(autopilot_model
    planner/src/action.cpp
    planner/src/state.cpp
    planner/src/simulated_environment.cpp
)
target_include_directories(autopilot_model PUBLIC planner/include)

include(CTest)
if(BUILD_TESTING)
    add_executable(phase1_tests planner/tests/test_phase1.cpp)
    target_link_libraries(phase1_tests PRIVATE autopilot_model GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(phase1_tests)
endif()
