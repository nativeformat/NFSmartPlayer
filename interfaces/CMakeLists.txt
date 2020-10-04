set(NATIVEFORMAT_INTERFACE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# temporary, haven't dealt with these on Linux yet
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_package(SWIG REQUIRED)

  add_subdirectory(csharp)

  add_subdirectory(python)
  add_subdirectory(objc)
endif()

if((NOT IOS) OR (NOT ANDROID))
  add_subdirectory(java)
endif()
