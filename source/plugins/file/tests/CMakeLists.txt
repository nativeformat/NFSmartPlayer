add_executable(FilePluginTests FilePluginTestRunner.cpp)
target_link_libraries(FilePluginTests
  FilePlugin
  NFSPLogger
  ${Boost_LIBRARIES})
target_include_directories(
  FilePluginTests
  PUBLIC
  ${NF_LOGGING_SCHEMA_SRC_DIR}
  ${FILEPLUGIN_INCLUDE_DIRECTORY})
