include(${SWIG_USE_FILE})
set(CMAKE_SWIG_FLAGS "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Generate C# interfaces
set(CMAKE_SWIG_FLAGS -w451)
set(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_SOURCE_DIR}")
set_source_files_properties(
  "${NATIVEFORMAT_INTERFACE_DIRECTORY}/NFSmartPlayer.i"
  PROPERTIES
  CPLUSPLUS ON)
swig_add_module(
  NFSmartPlayerCS
  csharp
  "${NATIVEFORMAT_INTERFACE_DIRECTORY}/NFSmartPlayer.i")
swig_link_libraries(NFSmartPlayerCS NFSmartPlayer ${CSHARP_LIBRARIES})
