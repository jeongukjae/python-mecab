option(BUILD_COV "Build for code coverage" OFF)

if(BUILD_COV AND CMAKE_BUILD_TYPE MATCHES Debug AND APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 --coverage")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
endif()
