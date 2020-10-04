add_executable(WavePluginTests WavePluginTestRunner.cpp SineWavePluginTest.cpp)
target_link_libraries(WavePluginTests
  WavePlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  WavePluginTests
  PUBLIC
  "${WAVEPLUGIN_INCLUDE_DIRECTORY}")
