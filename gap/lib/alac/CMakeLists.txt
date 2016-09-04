cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

project(ALAC)


set(SOURCES
  alac.c
)

set(PUBLIC_HEADERS
  alac.h)

add_library(gap_alac OBJECT ${SOURCES} ${PUBLIC_HEADERS} )
#target_include_directories(gap_alac INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})






