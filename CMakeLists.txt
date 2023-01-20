cmake_minimum_required(VERSION 3.9)
project (TestCGAL)

find_package(CGAL CONFIG REQUIRED)
find_package(OpenMP)
find_package(TBB CONFIG REQUIRED)
include(CGAL_TBB_support)

add_executable(TestCGAL segmentation_example.cpp)

target_link_libraries(TestCGAL PRIVATE TBB::tbb)
target_link_libraries(TestCGAL PRIVATE CGAL::TBB_support)
if(OpenMP_CXX_FOUND)
    target_link_libraries(TestCGAL PUBLIC OpenMP::OpenMP_CXX)
endif()

