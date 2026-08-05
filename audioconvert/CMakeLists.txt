cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
cmake_policy(VERSION 3.23...4.2)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

project(AUDIOCONVERT VERSION 1.3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

TEST_BIG_ENDIAN(AUDIOCONVERT_BIGENDIAN)

# TagLib - Audio metadata library (required, >=1.9.0)
find_package(TagLib 1.9.0 REQUIRED)

set(HEADERS AudioConvert.h
            AudioTools.h
            fox.h)

set(SOURCES AudioConvert.cpp
            AudioTools.cpp
            main.cpp)


configure_file(acconfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/acconfig.h)

#-------------------------------------------------------------------------------
#
# Workaround to compile with earlier versions of FOX, something
# that cannot be easily handled using the preprocessor
#
if (DEFINED FOX_VERSION AND FOX_VERSION VERSION_LESS "1.7.80")
    message("Replacing occurences of FXString::compare")
    foreach(sourcefile IN LISTS SOURCES)
        if(EXISTS ${sourcefile})
            file(READ ${sourcefile} content)
            string(REPLACE "FXString::compare" "FX::compare" updated "${content}")
            string(REPLACE "comparenatural" "compareversion" updated "${updated}")
            file(WRITE ${sourcefile} "${updated}")
        endif()
    endforeach()
endif()


add_executable(audioconvert ${SOURCES} ${HEADERS})

target_precompile_headers(audioconvert PRIVATE
        <fx.h>
)

# Link libraries using modern imported targets
# Imported targets automatically provide include directories
target_link_libraries(audioconvert PRIVATE
        gmtag
)

