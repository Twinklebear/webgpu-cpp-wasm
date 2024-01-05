# Load a binary file and output it as a C array in a header file. Set the
# variables:
#
# BIN_TO_H_INPUT_FILES: a comma separated list of files to embed
#
# BIN_TO_H_HEADER_FILE: output header file name

# BIN_TO_H_CPP_FILE: output cpp file name
#
# BIN_TO_H_APPEND: set to 1/true if the script should just append to an existing
# file and not write the #pragma once and include headers
#
function(bin_to_h)
  string(REPLACE "," ";" INPUT_FILES "${BIN_TO_H_INPUT_FILES}")

  # Create the header and CPP files
  file(WRITE ${BIN_TO_H_HEADER_FILE} "#pragma once\n")
  file(APPEND ${BIN_TO_H_HEADER_FILE} "#include <cstdint>\n")
  file(APPEND ${BIN_TO_H_HEADER_FILE} "#include <cstddef>\n")

  file(WRITE ${BIN_TO_H_CPP_FILE} "#include \"${BIN_TO_H_HEADER_FILE}\"\n")

  # Now loop through and embed each file
  foreach(FIN IN LISTS INPUT_FILES)
    get_filename_component(VAR_NAME ${FIN} NAME_WE)
    get_filename_component(VAR_NAME_SUFFIX ${FIN} EXT)
    string(REPLACE "." "_" VAR_NAME_SUFFIX ${VAR_NAME_SUFFIX})
    set(VAR_NAME "${VAR_NAME}${VAR_NAME_SUFFIX}")

    file(READ ${FIN} BINARY_CONTENT HEX)
    string(LENGTH "${BINARY_CONTENT}" HEX_STRING_LENGTH)
    math(EXPR BINARY_SIZE "${HEX_STRING_LENGTH} / 2" OUTPUT_FORMAT DECIMAL)

    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," ARRAY_CONTENT
                         ${BINARY_CONTENT})
    # Remove the extra comma at the end
    string(REGEX REPLACE ",$" "" ARRAY_CONTENT ${ARRAY_CONTENT})
    file(APPEND ${BIN_TO_H_HEADER_FILE}
         "constexpr size_t ${VAR_NAME}_size = ${BINARY_SIZE};\n")
    file(APPEND ${BIN_TO_H_HEADER_FILE}
         "extern const uint8_t ${VAR_NAME}[${VAR_NAME}_size];\n")

    file(APPEND ${BIN_TO_H_CPP_FILE}
         "const uint8_t ${VAR_NAME}[${VAR_NAME}_size] = {")
    file(APPEND ${BIN_TO_H_CPP_FILE} ${ARRAY_CONTENT})
    file(APPEND ${BIN_TO_H_CPP_FILE} "};\n")
  endforeach()
endfunction()

bin_to_h()
