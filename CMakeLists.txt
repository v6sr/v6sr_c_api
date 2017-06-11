cmake_minimum_required( VERSION 3.1 )
project( SegmentRouting )

# Build the API library
include_directories( ${PROJECT_SOURCE_DIR}/sr_api/include )

add_subdirectory(  ${PROJECT_SOURCE_DIR}/sr_api )
