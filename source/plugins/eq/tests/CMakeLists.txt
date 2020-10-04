
add_executable(EQPluginTests EQPluginTestRunner.cpp EQPluginTest.cpp)
target_link_libraries(EQPluginTests
  EQPlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  EQPluginTests
  PUBLIC
  "${EQPLUGIN_INCLUDE_DIRECTORY}")
