include(${SWIG_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Generate Python interfaces
find_package(PythonLibs)
include_directories(${PYTHON_INCLUDE_PATH})
set(CMAKE_SWIG_FLAGS -w451 "-DV8_VERSION=${V8_VERSION_HEX}")
set(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_SOURCE_DIR}")
set_source_files_properties(
  "${NATIVEFORMAT_INTERFACE_DIRECTORY}/NFSmartPlayer.i"
  PROPERTIES
  CPLUSPLUS ON)
swig_add_library(
  NFSmartPlayerPython
  LANGUAGE python
  SOURCES "${NATIVEFORMAT_INTERFACE_DIRECTORY}/NFSmartPlayer.i")
swig_link_libraries(NFSmartPlayerPython NFSmartPlayer ${PYTHON_LIBRARIES})
