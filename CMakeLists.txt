cmake_minimum_required(VERSION 3.1)

# option
option(BUILD_TEST "Build the tests" OFF)
option(BUILD_COV "Build for code coverage" OFF)

project(mecab VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/extensions/python/mecab/lib)

if(NOT CMAKE_BUILD_TYPE)
  # Release or Debug
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(BUILD_COV AND CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 --coverage")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
endif()

include_directories(./include)
set(TEST_CODE
      tests/test_iconv.cc
      tests/test_mmap.cc
      tests/test_param.cc
      tests/test_utils.cc
      tests/utils/test_string_utils.cc)
set(INTEGRATION_TEST_CODE
      tests-integration/test_cost_train.cc
      tests-integration/test_dics.cc
      tests-integration/test_eval.cc)

find_package(Iconv REQUIRED)

if(BUILD_TEST)
  enable_testing()
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/googletest")

  include_directories(${PROJECT_SOURCE_DIR}/include)
  include_directories(${gtest_SOURCE_DIR}/include)
  include_directories(${gmock_SOURCE_DIR}/include)

  add_executable(run-test ${TEST_CODE})
  target_link_libraries(run-test ${Iconv_LIBRARIES})
  target_link_libraries(run-test gtest gmock gtest_main)

  add_executable(run-integration ${INTEGRATION_TEST_CODE})
  target_link_libraries(run-integration ${Iconv_LIBRARIES})
  target_link_libraries(run-integration gtest gmock gtest_main)

  add_test(NAME run-test COMMAND ./run-test)
  add_test(NAME run-integration COMMAND ./run-integration)
endif()
