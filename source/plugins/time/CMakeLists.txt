set(TIMEPLUGIN_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(TIME_SOURCE_FILES
  LoopPlugin.h
  LoopPlugin.cpp
  TimePluginFactory.h
  TimePluginFactory.cpp
  StretchPlugin.h
  StretchPlugin.cpp)

set(TIME_LINK_LIBS
  ${Boost_LIBRARIES}
  ${ELASTIQUE_LIBRARIES}
  NFDriver
  ${COMMON_PLUGIN_LIBS})

add_library(TimePlugin STATIC ${TIME_SOURCE_FILES})

target_include_directories(
  TimePlugin
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS}
  PRIVATE
  ${ELASTIQUE_INCLUDE_DIR})
target_link_libraries(TimePlugin
  PUBLIC
  ${TIME_LINK_LIBS})
add_subdirectory(tests)
