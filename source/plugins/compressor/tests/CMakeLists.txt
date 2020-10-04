
add_executable(CompressorPluginTests
  CompressorPluginTestRunner.cpp
  CompressorPluginTest.cpp)
target_link_libraries(CompressorPluginTests
  CompressorPlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  CompressorPluginTests
  PUBLIC
  "${COMPRESSORPLUGIN_INCLUDE_DIRECTORY}")
