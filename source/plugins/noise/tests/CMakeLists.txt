
add_executable(NoisePluginTests NoisePluginTestRunner.cpp NoisePluginTest.cpp)
target_link_libraries(NoisePluginTests
  NoisePlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  NoisePluginTests
  PUBLIC
  "${NOISEPLUGIN_INCLUDE_DIRECTORY}")
