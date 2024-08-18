include(ExternalProject)

ExternalProject_Add(
  tbb_build
  PREFIX tbb_build
  GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
  GIT_TAG v2021.13.0
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
             -DCMAKE_CROSSCOMPILING_EMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}
             -DTBB_STRICT=OFF
             -DCMAKE_CXX_FLAGS=-Wno-unused-command-line-argument
             -DBUILD_SHARED_LIBS=ON
             -DTBB_EXAMPLES=OFF
             -DTBB_TEST=OFF
             -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/tbb_build)

add_library(tbb INTERFACE)
add_dependencies(tbb tbb_build)
ExternalProject_Get_Property(tbb_build INSTALL_DIR)
target_include_directories(tbb INTERFACE "${INSTALL_DIR}/include")

if(EMSCRIPTEN)
  # The emscripten build says "build shared libs", but emscripten will always
  # output static libs
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
