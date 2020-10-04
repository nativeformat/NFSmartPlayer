add_executable(PluginUtilTests PluginUtilTestRunner.cpp PluginUtilTests.cpp)
target_link_libraries(PluginUtilTests
  PluginUtil
  ${Boost_LIBRARIES})
target_include_directories(
  PluginUtilTests
  PUBLIC
  "${Boost_INCLUDE_DIR}"
  "${PLUGIN_UTIL_INCLUDE_DIRECTORY}")
