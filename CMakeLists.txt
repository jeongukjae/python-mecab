cmake_minimum_required(VERSION 3.1)

project(mecab VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/mecab/lib)

if(NOT CMAKE_BUILD_TYPE)
  # Release or Debug
  set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(./Includes)
set(SOURCE_CODE
      Sources/char_property.cc
      Sources/connector.cc
      Sources/context_id.cc
      Sources/dictionary.cc
      Sources/dictionary_rewriter.cc
      Sources/feature_index.cc
      Sources/iconv_utils.cc
      Sources/lbfgs.cc
      Sources/learner_tagger.cc
      Sources/libmecab.cc
      Sources/nbest_generator.cc
      Sources/param.cc
      Sources/string_buffer.cc
      Sources/tagger.cc
      Sources/tokenizer.cc
      Sources/utils.cc
      Sources/viterbi.cc
      Sources/writer.cc)
set(TEST_CODE
      tests/cc/test_utils.cc)

find_package(Iconv REQUIRED)

include(./cmake/Testing.cmake)
include(./cmake/CodeCoverage.cmake)

add_library(mecab SHARED ${SOURCE_CODE})
target_link_libraries(mecab ${Iconv_LIBRARIES})
