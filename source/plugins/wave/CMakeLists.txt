set(WAVEPLUGIN_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(
  WavePlugin
  STATIC
  SineWavePlugin.h
  SineWavePlugin.cpp
  WavePluginFactory.h
  WavePluginFactory.cpp)
target_include_directories(
  WavePlugin
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS})
target_link_libraries(WavePlugin
  ${Boost_LIBRARIES}
  ${COMMON_PLUGIN_LIBS})
add_subdirectory(tests)
