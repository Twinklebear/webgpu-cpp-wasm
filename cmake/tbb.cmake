include(ExternalProject)

ExternalProject_Add(
  tbb_build
  PREFIX tbb_build
  GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
  GIT_TAG 2d516c871b79d81032f0ca44d76e5072bc62d479
  # GIT_SHALLOW ON We need to run the emcmake cmake wrapper to build
  CMAKE_COMMAND $<$<BOOL:${EMSCRIPTEN}>:emcmake>
  ${CMAKE_COMMAND}
  # Emscripten build flags for TBB from
  # https://github.com/oneapi-src/oneTBB/blob/master/WASM_Support.md
  CMAKE_ARGS -DTBB_STRICT=OFF
             -DCMAKE_CXX_FLAGS=-Wno-unused-command-line-argument
             $<$<BOOL:${EMSCRIPTEN}>:-DTBB_DISABLE_HWLOC_AUTOMATIC_SEARCH=ON>
             -DBUILD_SHARED_LIBS=ON
             -DTBB_EXAMPLES=OFF
             -DTBB_TEST=OFF
             -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/tbb_build)

add_library(tbb INTERFACE)
add_dependencies(tbb tbb_build)
ExternalProject_Get_Property(tbb_build INSTALL_DIR)
target_include_directories(tbb INTERFACE "${INSTALL_DIR}/include")

if(EMSCRIPTEN)
  target_link_libraries(
    tbb
    INTERFACE
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tbb${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tbbmalloc${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tbbmalloc_proxy${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
else()
  target_link_libraries(
    tbb
    INTERFACE
      ${INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}tbb${CMAKE_SHARED_LIBRARY_SUFFIX}
      ${INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}tbbmalloc${CMAKE_SHARED_LIBRARY_SUFFIX}
      ${INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}tbbmalloc_proxy${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
endif()
