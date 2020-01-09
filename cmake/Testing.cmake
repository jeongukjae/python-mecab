option(BUILD_TEST "Build the tests" OFF)

if(BUILD_TEST AND CMAKE_BUILD_TYPE MATCHES Debug)
  enable_testing()
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/googletest")

  include_directories(${PROJECT_SOURCE_DIR}/include)
  include_directories(${gtest_SOURCE_DIR}/include)
  include_directories(${gmock_SOURCE_DIR}/include)

  add_executable(run-test ${TEST_CODE})
  target_link_libraries(run-test mecab)
  target_link_libraries(run-test gtest gmock gtest_main)

  add_test(NAME run-test COMMAND ./run-test)
endif()
