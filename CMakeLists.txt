cmake_policy(VERSION 3.2)
cmake_minimum_required(VERSION 3.2)

#set(CMAKE_VERBOSE_MAKEFILE TRUE)

# libdir name to allow overriding to lib32 / lib64
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Specify the name of the directory where libraries are meant to be installed")

# Override default build type
SET(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Specify the desired build type.")

# Override default install to /usr
set(CMAKE_INSTALL_PREFIX /usr CACHE STRING "Installation prefix used by install.")

# Add Goggles Music Manager
add_subdirectory(src)

# Extra Installation Files
install(PROGRAMS extra/import_banshee_stats.py extra/import_gogglesmm12.py extra/import_gogglesmm12_stats.py DESTINATION share/gogglesmm)
install(FILES icons/gogglesmm_16.png DESTINATION share/icons/hicolor/16x16/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_22.png DESTINATION share/icons/hicolor/22x22/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_24.png DESTINATION share/icons/hicolor/24x24/apps RENAME gogglesmm.png)
install(FILES icons/gogglesmm_32.png DESTINATION share/icons/hicolor/32x32/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_48.png DESTINATION share/icons/hicolor/48x48/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm.svg DESTINATION share/icons/hicolor/scalable/apps RENAME gogglesmm.svg)
install(FILES extra/gogglesmm.appdata.xml DESTINATION share/appdata)
install(FILES extra/gogglesmm.desktop DESTINATION share/applications)
install(FILES extra/gogglesmm.1 DESTINATION share/man/man1)

# Install translations when native language support has been enabled
if(WITH_NLS)
    set(LINGUAS "All" CACHE STRING "A space-seperated list of translations to compile in to gogglesmm, or \"None\".")
    if (LINGUAS STREQUAL "All")
        file(GLOB pofiles po/*.po)
        foreach(pofile ${pofiles})
            get_filename_component(lang ${pofile} NAME_WE)
            list(APPEND LANGUAGES ${lang})
        endforeach()
    else()
        if(NOT LINGUAS OR LINGUAS STREQUAL "None")
            set(LANGUAGES "")
        else()
            string(REGEX MATCHALL [a-zA-Z_@]+ LANGUAGES ${LINGUAS})
        endif()
    endif()

    foreach(lang ${LANGUAGES})
        install(FILES po/${lang}.mo DESTINATION share/locale/${lang}/LC_MESSAGES RENAME gogglesmm.mo)
    endforeach()
endif()



