set(CHANNELPLUGIN_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(
  ChannelPlugin
  STATIC
  PluginFactory.h
  PluginFactory.cpp
  ChannelPlugin.h
  ChannelPlugin.cpp)
target_include_directories(
  ChannelPlugin
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS})
target_link_libraries(
  ChannelPlugin
  ${Boost_LIBRARIES}
  ${COMMON_PLUGIN_LIBS})
add_subdirectory(tests)
