set(WAAPLUGIN_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(
  WAAPlugin
  STATIC
  GainPlugin.h
  GainPlugin.cpp
  WAAPluginFactory.h
  WAAPluginFactory.cpp
  DelayPlugin.h
  DelayPlugin.cpp)
target_include_directories(
  WAAPlugin
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR})

set(LIBRARIES ${COMMON_PLUGIN_LIBS})
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_library(ACCELERATE_FRAMEWORK Accelerate)
  list(APPEND LIBRARIES ${ACCELERATE_FRAMEWORK})
endif()
target_link_libraries(
  WAAPlugin
  ${LIBRARIES})
add_subdirectory(tests)
