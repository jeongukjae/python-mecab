cmake_minimum_required(VERSION 3.1)

project(mecab VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/mecab/lib)

if(NOT CMAKE_BUILD_TYPE)
  # Release or Debug
  set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(./include)
set(SOURCE_CODE
      src/tagger.cc)
set(TEST_CODE
      tests/cc/test_iconv.cc
      tests/cc/test_mmap.cc
      tests/cc/test_param.cc
      tests/cc/test_utils.cc)

find_package(Iconv REQUIRED)

include(./cmake/Testing.cmake)
include(./cmake/CodeCoverage.cmake)

add_library(mecab SHARED ${SOURCE_CODE})
target_link_libraries(mecab ${Iconv_LIBRARIES})
