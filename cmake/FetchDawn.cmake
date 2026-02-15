# Based on Elie Michel's Dawn Fetch/Build CMake script
# https://github.com/eliemichel/WebGPU-distribution/blob/main/dawn/FetchDawnSource.cmake

# Prevent multiple includes
if(TARGET dawn_native)
  return()
endif()

# Override Dawn default cache variables with a more minimalistic choice
# of backend and the fetch script instead of depot_tools.
option(DAWN_FETCH_DEPENDENCIES "Use fetch_dawn_dependencies.py as an alternative to using depot_tools" ON)

if(APPLE)
  set(ENABLE_VULKAN OFF)
  set(ENABLE_METAL ON)
  set(ENABLE_D3D12 OFF)
elseif(WIN32)
  set(ENABLE_VULKAN OFF)
  set(ENABLE_METAL OFF)
  set(ENABLE_D3D12 ON)
else()
  set(ENABLE_VULKAN ON)
  set(ENABLE_METAL OFF)
  set(ENABLE_D3D12 OFF)
endif()
option(DAWN_ENABLE_D3D11 "Enable compilation of the D3D11 backend" OFF)
option(DAWN_ENABLE_D3D12 "Enable compilation of the D3D12 backend" ${ENABLE_D3D12})
option(DAWN_ENABLE_METAL "Enable compilation of the Metal backend" ${ENABLE_METAL})
option(DAWN_ENABLE_NULL "Enable compilation of the Null backend" OFF)
option(DAWN_ENABLE_DESKTOP_GL "Enable compilation of the OpenGL backend" OFF)
option(DAWN_ENABLE_OPENGLES "Enable compilation of the OpenGL ES backend" OFF)
option(DAWN_ENABLE_VULKAN "Enable compilation of the Vulkan backend" ${ENABLE_VULKAN})
option(TINT_BUILD_SPV_READER "Build the SPIR-V input reader" OFF)
option(DAWN_BUILD_SAMPLES "Enables building Dawn's samples" OFF)
option(DAWN_BUILD_TESTS "Enables building Dawn's tests" OFF)
option(TINT_BUILD_CMD_TOOLS "Build the Tint command line tools" OFF)
option(TINT_BUILD_IR_BINARY "Build IR binary format support" OFF)
option(DAWN_USE_GLFW "Use GLFW" OFF)

include(FetchContent)
find_package(Python3 REQUIRED)

set(DAWN_VERSION "7688" CACHE STRING "Dawn chromium version")
set(DAWN_MIRROR "https://dawn.googlesource.com/dawn" CACHE STRING "Dawn git mirror")

FetchContent_Declare(
  dawn
  # Manual download mode, even shallower than GIT_SHALLOW ON
  DOWNLOAD_COMMAND
    cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
    git init &&
    git fetch --depth=1 ${DAWN_MIRROR} chromium/${DAWN_VERSION} &&
    git reset --hard FETCH_HEAD)

FetchContent_MakeAvailable(dawn)

set(AllDawnTargets
    core_tables
    dawn_common
    dawn_glfw
    dawn_headers
    dawn_native
    dawn_platform
    dawn_proc
    dawn_wire
    dawn_native_objects
    dawn_shared_utils
    partition_alloc
    dawncpp
    dawncpp_headers
    enum_string_mapping
    extinst_tables
    webgpu_dawn
    webgpu_headers_gen
    tint-format
    tint-lint
    tint_api
    tint_api_common
    tint_cmd_common
    tint_lang_core
    tint_lang_core_common
    tint_lang_core_constant
    tint_lang_core_intrinsic
    tint_lang_core_ir
    tint_lang_core_ir_analysis
    tint_lang_core_ir_transform
    tint_lang_core_ir_transform_common
    tint_lang_core_ir_type
    tint_lang_core_type
    tint_lang_glsl_validate
    tint_lang_hlsl_writer_common
    tint_lang_hlsl_writer_helpers
    tint_lang_hlsl_writer_printer
    tint_lang_hlsl_writer_raise
    tint_lang_msl
    tint_lang_msl_intrinsic
    tint_lang_msl_ir
    tint_lang_spirv
    tint_lang_spirv_intrinsic
    tint_lang_spirv_ir
    tint_lang_spirv_reader_lower
    tint_lang_spirv_type
    tint_lang_spirv_validate
    tint_lang_spirv_writer
    tint_lang_spirv_writer_common
    tint_lang_spirv_writer_helpers
    tint_lang_spirv_writer_printer
    tint_lang_spirv_writer_raise
    tint_lang_wgsl
    tint_lang_wgsl_ast
    tint_lang_wgsl_ast_transform
    tint_lang_wgsl_common
    tint_lang_wgsl_features
    tint_lang_wgsl_helpers
    tint_lang_wgsl_inspector
    tint_lang_wgsl_intrinsic
    tint_lang_wgsl_ir
    tint_lang_wgsl_program
    tint_lang_wgsl_reader
    tint_lang_wgsl_reader_lower
    tint_lang_wgsl_reader_parser
    tint_lang_wgsl_reader_program_to_ir
    tint_lang_wgsl_resolver
    tint_lang_wgsl_sem
    tint_lang_wgsl_writer
    tint_lang_wgsl_writer_ast_printer
    tint_lang_wgsl_writer_ir_to_program
    tint_lang_wgsl_writer_raise
    tint_lang_wgsl_writer_syntax_tree_printer
    tint_utils
    tint_utils_bytes
    tint_utils_command
    tint_utils_containers
    tint_utils_diagnostic
    tint_utils_file
    tint_utils_ice
    tint_utils_macros
    tint_utils_math
    tint_utils_memory
    tint_utils_rtti
    tint_utils_strconv
    tint_utils_symbol
    tint_utils_system
    tint_utils_text
    tint_utils_text_generator)

foreach(Target ${AllDawnTargets})
  if(TARGET ${Target})
    get_property(AliasedTarget TARGET "${Target}" PROPERTY ALIASED_TARGET)
    if("${AliasedTarget}" STREQUAL "")
      set_property(TARGET ${Target} PROPERTY FOLDER "Dawn")
    endif()
  else()
    message(STATUS "NB: '${Target}' is no longer a target of the Dawn project.")
  endif()
endforeach()

add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE webgpu_dawn)
target_include_directories(webgpu
                           INTERFACE ${dawn_SOURCE_DIR}/include)
