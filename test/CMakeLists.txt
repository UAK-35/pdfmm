set(PDF_TEST_RESOURCE_PATH "${PROJECT_SOURCE_DIR}/extern/resources")
if(NOT EXISTS "${PDF_TEST_RESOURCE_PATH}")
    message(FATAL_ERROR "Resources path doesn't exists: try fetch git submodules")
endif()

set(PDF_TEST_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/out")
make_directory(${PDF_TEST_OUTPUT_PATH})

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/include")
include_directories(common)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
set(Catch2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")
find_package(Catch2 REQUIRED)
add_subdirectory(unit)
