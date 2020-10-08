message(STATUS "NFSmartPlayer source: BOOST Info")
message(STATUS "NFSmartPlayer source: Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR}")
message(STATUS "NFSmartPlayer source: Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")
message(STATUS "NFSmartPlayer source: Boost_LIBRARIES ${Boost_LIBRARIES}")
message(STATUS "NFSmartPlayer source: Boost_FOUND ${Boost_FOUND}")
message(STATUS "NFSmartPlayer source: BOOST_ROOT ${BOOST_ROOT}")

set(NFSMARTPLAYER_INCLUDE_DIRS
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}
  ${NFLOGGER_INCLUDE_DIRECTORY})

set(NFSMARTPLAYER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(plugins)

# Add the player library
add_library(
  NFSmartPlayer
  STATIC
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Plugin.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Registry.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Factory.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/nf_smart_player.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Client.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Script.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Graph.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Node.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Edge.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/ErrorCode.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/Content.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/CallbackTypes.h
  ${NFSMARTPLAYER_INCLUDE_DIRECTORY}/NFSmartPlayer/GlobalLogger.h
  Player.cpp
  Registry.cpp
  nf_smart_player.cpp
  Limiter.h
  Limiter.cpp
  NFOSCHandler.h
  NFOSCHandler.cpp
  Client.cpp
  ClientImplementation.h
  ClientImplementation.cpp
  GraphImplementation.h
  GraphImplementation.cpp
  Notification.h
  Notification.cpp
  ScriptImplementation.h
  ScriptImplementation.cpp
  ScriptDelegate.h
  Player.h
  NodeImplementation.h
  NodeImplementation.cpp
  Node.cpp
  Graph.cpp
  EdgeImplementation.h
  EdgeImplementation.cpp
  Edge.cpp
  ErrorCode.cpp
  GraphDelegate.h
  GlobalLogger.cpp
  CallbackTypes.cpp
  Authoriser.h
  MixerPlugin.h
  MixerPlugin.cpp)
target_compile_definitions(NFSmartPlayer PUBLIC -DNF_LOG_ERROR=1)
target_include_directories(
  NFSmartPlayer
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS}
  ${NFSMARTPLAYER_LIBRARIES_DIRECTORY}/NFDriver/include
  ${NFSMARTPLAYER_LIBRARIES_DIRECTORY}/duktape/src
  ${NFSMARTPLAYER_LIBRARIES_DIRECTORY}/NFLogger/include
  ${NFSMARTPLAYER_OUTPUT_DIRECTORY})
set(NFSP_LIBS NFDriver
              duktape
              NFDecoder
              WAAPlugin
              ${Boost_LIBRARIES}
              NoisePlugin
              WavePlugin
              oscpack
              FilePlugin
              ChannelPlugin
              CompressorPlugin
              EQPlugin
              TimePlugin
              NFHTTP
              NFLogger
              NFGrapherParam
              nlohmann_json)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(NFSmartPlayer ${NFSP_LIBS})
else()
  target_link_libraries(NFSmartPlayer -Wl,--start-group
                                      ${NFSP_LIBS}
                                      -Wl,--end-group)
endif()

add_library(NFSPLogger
  ${NFSMARTPLAYER_SOURCE_DIR}/CallbackTypes.cpp
)
target_compile_definitions(NFSPLogger PUBLIC -DNF_LOG_ERROR=1)
target_include_directories(NFSPLogger
  PUBLIC
  ${NFSMARTPLAYER_INCLUDE_DIRS}
  ${NFSMARTPLAYER_LIBRARIES_DIRECTORY}/NFLogger/include)
target_link_libraries(NFSPLogger NFLogger nlohmann_json)
install(TARGETS NFSmartPlayer DESTINATION /usr/lib)

add_subdirectory(cli)
add_subdirectory(tests)
