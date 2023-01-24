project (TestCGAL)

cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CGAL CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

include(CGAL_TBB_support)

add_executable(TestCGAL segmentation_example.cpp)

target_link_libraries(TestCGAL PRIVATE TBB::tbb)
target_link_libraries(TestCGAL PRIVATE CGAL::TBB_support)


