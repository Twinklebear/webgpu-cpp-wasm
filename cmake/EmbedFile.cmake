# find_file(BINTOH NAME BinToH.cmake PATHS ${CMAKE_MODULE_PATH}
# ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(BINTOH "${CMAKE_CURRENT_LIST_DIR}/BinToH.cmake" ABSOLUTE)

function(embed_files)
  cmake_parse_arguments(PARSE_ARGV 1 EMBED_FILE "" "" "${options}")

  # Resolve the full path to each file
  set(EMBED_FILE_LIST "")
  foreach(FIN IN LISTS EMBED_FILE_UNPARSED_ARGUMENTS)
    get_filename_component(FULL_FILE_PATH "${CMAKE_CURRENT_LIST_DIR}/${FIN}"
                           ABSOLUTE)
    list(APPEND EMBED_FILE_LIST ${FULL_FILE_PATH})

    get_filename_component(FNAME ${FULL_FILE_PATH} NAME_WE)
  endforeach()

  string(REPLACE ";" "," EMBED_FILE_STR "${EMBED_FILE_LIST}")

  set(EMBED_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.h")
  set(EMBED_CPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ARGV0}.cpp")
  add_custom_command(
    OUTPUT ${EMBED_CPP_FILE}
    COMMAND
      ${CMAKE_COMMAND} -DBIN_TO_H_INPUT_FILES=${EMBED_FILE_STR}
      -DBIN_TO_H_HEADER_FILE=${EMBED_HEADER_FILE}
      -DBIN_TO_H_CPP_FILE=${EMBED_CPP_FILE} -P ${BINTOH}
    DEPENDS ${EMBED_FILE_LIST}
    COMMENT "Embedding ${EMBED_FILE_UNPARSED_ARGUMENTS} in ${EMBED_HEADER_FILE}"
  )

  add_library(${ARGV0} ${EMBED_CPP_FILE})
  target_include_directories(${ARGV0} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  set_target_properties(${ARGV0} PROPERTIES CXX_STANDARD 20
                                            CXX_STANDARD_REQUIRED ON)
endfunction()
