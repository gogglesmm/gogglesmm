# Translations.cmake
#
# Handles gettext translation files for the gogglesmm application:
#   - Installation of compiled .mo files
#   - Custom targets for updating .pot/.po files (manual invocation)
#
# Usage:
#   include(Translations)
#
#   # Install translations
#   install_translations(
#     DOMAIN gogglesmm
#     PO_DIR ${CMAKE_SOURCE_DIR}/po
#     LANGUAGES de es fr hu ko pt ru  # Optional, defaults to all available
#   )
#
#   # Create update-pot target (optional)
#   add_pot_update_target(
#     DOMAIN gogglesmm
#     POT_FILE ${CMAKE_SOURCE_DIR}/po/gogglesmm.pot
#     SOURCE_FILES src/*.cpp src/*.h
#     KEYWORDS tr:1 fxtr:1 notr:1 fxtrformat:1
#     FLAGS fxtrformat:1:c-format
#     PACKAGE_VERSION ${PROJECT_VERSION}
#     BUGS_ADDRESS s.jansen@gmail.com
#     COPYRIGHT_HOLDER "Sander Jansen"
#   )
#
# Options:
#   LINGUAS cache variable controls which translations to install:
#     - "All" (default): Install all available translations
#     - "None": Don't install any translations
#     - Space-separated list: Install specific languages (e.g., "de fr es")

# Store the directory of this module file for use by functions
# This must be at module level since CMAKE_CURRENT_LIST_DIR changes when functions are called
set(_TRANSLATIONS_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Function to install gettext translation files
function(install_translations)
  cmake_parse_arguments(
    TRANS                          # Prefix for parsed arguments
    ""                             # Options (boolean flags)
    "DOMAIN;PO_DIR"               # Single-value arguments
    "LANGUAGES"                    # Multi-value arguments
    ${ARGN}
  )

  # Validate required arguments
  if(NOT TRANS_DOMAIN)
    message(FATAL_ERROR "install_translations: DOMAIN is required")
  endif()

  if(NOT TRANS_PO_DIR)
    message(FATAL_ERROR "install_translations: PO_DIR is required")
  endif()

  # Default to all languages if none specified
  if(NOT TRANS_LANGUAGES)
    # Find all .po files in the directory
    file(GLOB _po_files "${TRANS_PO_DIR}/*.po")
    foreach(_po_file ${_po_files})
      get_filename_component(_lang ${_po_file} NAME_WE)
      list(APPEND TRANS_LANGUAGES ${_lang})
    endforeach()
  endif()

  # Check LINGUAS cache variable for user preferences
  set(LINGUAS "All" CACHE STRING "Translations to install: 'All', 'None', or space-separated language codes")

  # Determine which languages to install
  if(LINGUAS STREQUAL "All")
    set(_languages_to_install ${TRANS_LANGUAGES})
    list(LENGTH _languages_to_install _count)
    message(STATUS "Translations: Installing all ${_count} available translations")
  elseif(LINGUAS STREQUAL "None" OR NOT LINGUAS)
    set(_languages_to_install "")
    message(STATUS "Translations: Skipping installation (LINGUAS=None)")
    return()
  else()
    # Parse space-separated list of languages
    string(REPLACE " " ";" _languages_to_install "${LINGUAS}")
    list(LENGTH _languages_to_install _count)
    message(STATUS "Translations: Installing ${_count} selected translations: ${_languages_to_install}")
  endif()

  # Install each translation
  set(_installed_count 0)
  set(_missing_languages "")

  foreach(_lang ${_languages_to_install})
    set(_mo_file "${TRANS_PO_DIR}/${_lang}.mo")

    if(EXISTS "${_mo_file}")
      install(
        FILES "${_mo_file}"
        DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${_lang}/LC_MESSAGES
        RENAME ${TRANS_DOMAIN}.mo
      )
      math(EXPR _installed_count "${_installed_count} + 1")
    else()
      list(APPEND _missing_languages ${_lang})
    endif()
  endforeach()

  # Report results
  if(_installed_count GREATER 0)
    message(STATUS "Translations: Configured ${_installed_count} translation(s) for installation")
  endif()

  if(_missing_languages)
    message(WARNING "Translations: Missing .mo files for languages: ${_missing_languages}")
  endif()
endfunction()

#===============================================================================
# Function to create custom target for updating .pot and .po files
#===============================================================================

function(add_pot_update_target)
  cmake_parse_arguments(
    POT                                    # Prefix
    "JOIN"                                 # Options (boolean)
    "DOMAIN;POT_FILE;PACKAGE_VERSION;BUGS_ADDRESS;COPYRIGHT_HOLDER"  # Single-value
    "SOURCE_FILES;KEYWORDS;FLAGS;APPEND_SOURCE_FILES"                 # Multi-value
    ${ARGN}
  )

  # Find gettext tools
  find_program(XGETTEXT_EXECUTABLE xgettext DOC "Path to xgettext executable")
  find_program(MSGMERGE_EXECUTABLE msgmerge DOC "Path to msgmerge executable")

  if(NOT XGETTEXT_EXECUTABLE)
    message(STATUS "Translations: xgettext not found, skipping update-pot target")
    return()
  endif()

  if(NOT MSGMERGE_EXECUTABLE)
    message(STATUS "Translations: msgmerge not found, skipping update-pot target")
    return()
  endif()

  # Validate required arguments
  if(NOT POT_DOMAIN)
    message(FATAL_ERROR "add_pot_update_target: DOMAIN is required")
  endif()

  if(NOT POT_POT_FILE)
    message(FATAL_ERROR "add_pot_update_target: POT_FILE is required")
  endif()

  if(NOT POT_SOURCE_FILES)
    message(FATAL_ERROR "add_pot_update_target: SOURCE_FILES is required")
  endif()

  # Get directory of POT file for .po files
  get_filename_component(_pot_dir "${POT_POT_FILE}" DIRECTORY)

  # Build xgettext command arguments
  set(_xgettext_args
    -C
    --from-code=UTF-8
    --output=${POT_POT_FILE}
  )

  if(POT_PACKAGE_VERSION)
    list(APPEND _xgettext_args --package-version=${POT_PACKAGE_VERSION})
  endif()

  if(POT_BUGS_ADDRESS)
    list(APPEND _xgettext_args --msgid-bugs-address=${POT_BUGS_ADDRESS})
  endif()

  if(POT_COPYRIGHT_HOLDER)
    list(APPEND _xgettext_args --copyright-holder=${POT_COPYRIGHT_HOLDER})
  endif()

  list(APPEND _xgettext_args --package-name=${POT_DOMAIN})

  # Add keywords
  foreach(_keyword ${POT_KEYWORDS})
    list(APPEND _xgettext_args --keyword=${_keyword})
  endforeach()

  # Add flags
  foreach(_flag ${POT_FLAGS})
    list(APPEND _xgettext_args --flag=${_flag})
  endforeach()

  # Expand glob patterns in source files (at configure time)
  set(_expanded_sources "")
  foreach(_pattern ${POT_SOURCE_FILES})
    file(GLOB _files ${_pattern})
    list(APPEND _expanded_sources ${_files})
  endforeach()

  # Convert absolute paths to relative paths for cleaner .pot/.po file comments
  set(_relative_sources "")
  foreach(_abs_path ${_expanded_sources})
    file(RELATIVE_PATH _rel_path "${CMAKE_SOURCE_DIR}" "${_abs_path}")
    list(APPEND _relative_sources ${_rel_path})
  endforeach()

  # Create custom target for updating .pot file
  add_custom_target(update-pot
    COMMAND ${XGETTEXT_EXECUTABLE} ${_xgettext_args} ${_relative_sources}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Extracting translatable strings to ${POT_POT_FILE}"
    VERBATIM
  )

  # If there are additional source files to append
  if(POT_APPEND_SOURCE_FILES)
    set(_append_sources "")
    foreach(_pattern ${POT_APPEND_SOURCE_FILES})
      file(GLOB _files ${_pattern})
      list(APPEND _append_sources ${_files})
    endforeach()

    # Convert absolute paths to relative paths for cleaner .pot/.po file comments
    set(_relative_append_sources "")
    foreach(_abs_path ${_append_sources})
      file(RELATIVE_PATH _rel_path "${CMAKE_SOURCE_DIR}" "${_abs_path}")
      list(APPEND _relative_append_sources ${_rel_path})
    endforeach()

    # Build append command (same args plus -j for join)
    set(_xgettext_append_args ${_xgettext_args} -j)

    add_custom_target(update-pot-append
      COMMAND ${XGETTEXT_EXECUTABLE} ${_xgettext_append_args} ${_relative_append_sources}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Appending additional translatable strings to ${POT_POT_FILE}"
      DEPENDS update-pot
      VERBATIM
    )
  endif()

  # Create custom target for updating .po files from .pot
  # Depends on update-pot-append (if append sources exist) or update-pot
  if(POT_APPEND_SOURCE_FILES)
    add_custom_target(update-po
      COMMAND ${CMAKE_COMMAND} -E echo "Updating .po files from ${POT_POT_FILE}..."
      COMMAND ${CMAKE_COMMAND}
        -DMSGMERGE_EXECUTABLE=${MSGMERGE_EXECUTABLE}
        -DPOT_FILE=${POT_POT_FILE}
        -DPO_DIR=${_pot_dir}
        -P ${_TRANSLATIONS_MODULE_DIR}/TranslationsUpdate.cmake
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Merging ${POT_POT_FILE} into .po files"
      DEPENDS update-pot-append
      VERBATIM
    )
  else()
    add_custom_target(update-po
      COMMAND ${CMAKE_COMMAND} -E echo "Updating .po files from ${POT_POT_FILE}..."
      COMMAND ${CMAKE_COMMAND}
        -DMSGMERGE_EXECUTABLE=${MSGMERGE_EXECUTABLE}
        -DPOT_FILE=${POT_POT_FILE}
        -DPO_DIR=${_pot_dir}
        -P ${_TRANSLATIONS_MODULE_DIR}/TranslationsUpdate.cmake
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Merging ${POT_POT_FILE} into .po files"
      DEPENDS update-pot
      VERBATIM
    )
  endif()

  message(STATUS "Translations: Created targets 'update-pot' and 'update-po'")
  message(STATUS "  xgettext: ${XGETTEXT_EXECUTABLE}")
  message(STATUS "  msgmerge: ${MSGMERGE_EXECUTABLE}")
  message(STATUS "  Run: cmake --build . --target update-po")
endfunction()
