add_executable(ChannelPluginTests
  ChannelPluginTestRunner.cpp
  ChannelPluginTest.cpp)

target_link_libraries(ChannelPluginTests
  ChannelPlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  ChannelPluginTests
  PUBLIC
  "${CHANNELPLUGIN_INCLUDE_DIRECTORY}")
