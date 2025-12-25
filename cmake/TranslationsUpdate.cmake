# TranslationsUpdate.cmake
#
# Helper script to update all .po files from a .pot file
# Invoked via: cmake -P TranslationsUpdate.cmake
#
# Required variables:
#   MSGMERGE_EXECUTABLE - Path to msgmerge
#   POT_FILE            - Path to .pot template file
#   PO_DIR              - Directory containing .po files

if(NOT MSGMERGE_EXECUTABLE)
  message(FATAL_ERROR "MSGMERGE_EXECUTABLE not defined")
endif()

if(NOT POT_FILE)
  message(FATAL_ERROR "POT_FILE not defined")
endif()

if(NOT PO_DIR)
  message(FATAL_ERROR "PO_DIR not defined")
endif()

# Find all .po files
file(GLOB po_files "${PO_DIR}/*.po")

if(NOT po_files)
  message(STATUS "No .po files found in ${PO_DIR}")
  return()
endif()

# Update each .po file
foreach(po_file ${po_files})
  get_filename_component(lang ${po_file} NAME)
  message(STATUS "Updating ${lang}...")

  execute_process(
    COMMAND ${MSGMERGE_EXECUTABLE} --update --backup=none ${po_file} ${POT_FILE}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )

  if(result)
    message(WARNING "Failed to update ${lang}: ${error}")
  endif()
endforeach()

message(STATUS "Translation update complete")
