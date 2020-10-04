
add_executable(TimePluginTests
  TimePluginTestRunner.cpp LoopPluginTest.cpp)
target_link_libraries(TimePluginTests
  TimePlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  TimePluginTests
  PUBLIC
  "${TIMEPLUGIN_INCLUDE_DIRECTORY}")
