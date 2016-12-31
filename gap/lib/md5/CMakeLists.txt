cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

project(MD5)


set(SOURCES
  md5.cpp
)

set(PUBLIC_HEADERS
  md5.h)

add_library(gap_md5 OBJECT ${SOURCES} ${PUBLIC_HEADERS} )
#target_include_directories(gap_alac INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})






