set(PLUGIN_UTIL_INCLUDE_DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}"
  CACHE STRING "Include directory for plugin utils" FORCE)
add_library(
  PluginUtil
  STATIC
  ButterFilter.h
  ButterFilter.cpp
  BandSplitter.h
  BandSplitter.cpp)
target_include_directories(
  PluginUtil
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS})

add_subdirectory(tests)
