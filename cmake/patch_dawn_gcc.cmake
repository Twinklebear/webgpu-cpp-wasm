# Patches for Dawn compatibility with GCC 13+

# Fix 1: template-id on destructor (ill-formed in C++20+, hard error on GCC 13+)
# ~EnumOption<E>() -> ~EnumOption()
set(FILE_PATH "${SOURCE_DIR}/src/dawn/utils/CommandLineParser.h")
if(EXISTS "${FILE_PATH}")
  file(READ "${FILE_PATH}" content)
  string(REPLACE "~EnumOption<E>()" "~EnumOption()" content "${content}")
  file(WRITE "${FILE_PATH}" "${content}")
endif()

# Fix 2: Qualify Sink/Source in stream::Stream specialization
# The specialization in TintUtils.h is in namespace dawn::native but Sink/Source
# are in dawn::native::stream. GCC requires explicit qualification.
set(FILE_PATH "${SOURCE_DIR}/src/dawn/native/TintUtils.h")
if(EXISTS "${FILE_PATH}")
  file(READ "${FILE_PATH}" content)
  string(REPLACE "static void Write(Sink* s," "static void Write(stream::Sink* s," content "${content}")
  string(REPLACE "static MaybeError Read(Source* s," "static MaybeError Read(stream::Source* s," content "${content}")
  file(WRITE "${FILE_PATH}" "${content}")
endif()
