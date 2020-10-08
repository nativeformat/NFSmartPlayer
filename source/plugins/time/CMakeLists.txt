set(TIMEPLUGIN_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(TIME_SOURCE_FILES
  LoopPlugin.h
  LoopPlugin.cpp
  TimePluginFactory.h
  TimePluginFactory.cpp
)

set(TIME_LINK_LIBS
  ${Boost_LIBRARIES}
  NFDriver
  ${COMMON_PLUGIN_LIBS})

if(WITH_ELASTIQUE)
  set(TIME_SOURCE_FILES ${TIME_SOURCE_FILES} StretchPlugin.h StretchPlugin.cpp)
  set(TIME_LINK_LIBS
    ${TIME_LINK_LIBS}
    ${ELASTIQUE_LIBRARIES}
  )
endif()

add_library(TimePlugin STATIC ${TIME_SOURCE_FILES})

target_include_directories(
  TimePlugin
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS}
)

if(WITH_ELASTIQUE)
  target_include_directories(
    TimePlugin
    PRIVATE
    ${ELASTIQUE_INCLUDE_DIR})
endif()

target_link_libraries(TimePlugin
  PUBLIC
  ${TIME_LINK_LIBS})

add_subdirectory(tests)
