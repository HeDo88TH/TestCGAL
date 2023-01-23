
project (TestCGAL)
cmake_minimum_required(VERSION 3.9)

add_definitions(-DCGAL_LINKED_WITH_LASLIB)

message(STATUS "${CMAKE_MODULE_PATH}")

find_package(LASlib REQUIRED)
find_package(OpenMP)
find_package(TBB CONFIG REQUIRED)
find_package(CGAL CONFIG REQUIRED)

include(CGAL_TBB_support)

add_executable(TestCGAL segmentation_example.cpp)

target_link_libraries(TestCGAL PUBLIC TBB::tbb CGAL::TBB_support CGAL::LASLIB_support LASlib)

if(OpenMP_CXX_FOUND)
    target_link_libraries(TestCGAL PUBLIC OpenMP::OpenMP_CXX)
endif()

