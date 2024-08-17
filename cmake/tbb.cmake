include(ExternalProject)

ExternalProject_Add(
  tbb_build
  PREFIX tbb_build
  GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
  GIT_TAG v2021.13.0
  GIT_SHALLOW ON
  # We need to run the emcmake cmake wrapper to build
  CMAKE_COMMAND $<${EMSCRIPTEN}:emcmake> cmake
  # Emscripten build flags for TBB from
  # https://github.com/oneapi-src/oneTBB/blob/master/WASM_Support.md
  CMAKE_ARGS -DTBB_STRICT=OFF
             -DCMAKE_CXX_FLAGS=-Wno-unused-command-line-argument
             $<${EMSCRIPTEN}:-DTBB_DISABLE_HWLOC_AUTOMATIC_SEARCH=ON>
             -DBUILD_SHARED_LIBS=ON
             -DTBB_EXAMPLES=OFF
             -DTBB_TEST=OFF
             -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/tbb_build)

add_library(tbb INTERFACE)
add_dependencies(tbb tbb_build)
ExternalProject_Get_Property(tbb_build INSTALL_DIR)
message("TBB Install dir = ${INSTALL_DIR}")
target_include_directories(tbb INTERFACE "${INSTALL_DIR}/include")
target_link_libraries(
  tbb
  INTERFACE
    ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tbb${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tbbmalloc${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tbbmalloc_proxy${CMAKE_STATIC_LIBRARY_SUFFIX}
)
