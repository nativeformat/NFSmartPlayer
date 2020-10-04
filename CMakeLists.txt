cmake_minimum_required(VERSION 3.5)

# Set CMAKE_BUILD_TYPE before calling project()
# Use existing cache string if present
if(DEFINED  CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING
    "Choose the type of build: None, Debug, or Release"
    FORCE)
else()
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
    "Choose the type of build: None, Debug, or Release"
    FORCE)
endif()

project(NFSmartPlayer)

# add_definitions(-DNF_LOG_ERROR)

if(TEST_LOG_SINK)
  add_definitions(-DTEST_LOG_SINK)
endif()

set(CMAKE_CXX_STANDARD 11)

set(
  CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} \
  -Wno-deprecated-declarations \
  -Wno-unused-local-typedefs \
  -Wno-enum-compare \
  -fPIC")

if(LLVM_STDLIB)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
  -Wno-tautological-undefined-compare \
  -Wno-shorten-64-to-32 \
  -stdlib=libc++ \
  -std=c++11")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
endif()

set(
  CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -fPIC")
if(CODE_COVERAGE)
  message("Using Code Coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
if(USE_ADDRESS_SANITIZER)
  message("Using Address Sanitizer")
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")
  set(
    CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")
endif()
if(USE_THREAD_SANITIZER)
  message("Using Thread Sanitizer")
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fsanitize=thread -g -fno-omit-frame-pointer")
  set(
    CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -g -fno-omit-frame-pointer")
endif()
if(USE_UNDEFINED_BEHAVIOUR_SANITIZER)
  message("Using Undefined Behaviour Sanitizer")
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fsanitize=undefined -g -fno-omit-frame-pointer")
  set(
    CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -g -fno-omit-frame-pointer")
endif()
set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS "armv7 armv7s arm64 i386 x86_64")

if(IOS)
  message("Building a project for iOS")
  set(CMAKE_XCODE_ATTRIBUTE_SDKROOT iphoneos)
  set(CMAKE_XCODE_SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
endif()

execute_process(COMMAND
  python tools/generate-version.py
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR})

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Don't build shared libraries" FORCE)

set(NFSMARTPLAYER_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(NFSMARTPLAYER_LIBRARIES_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libraries")
set(NFSMARTPLAYER_TOOLS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools")
set(NFSMARTPLAYER_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(NFSMARTPLAYER_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/output")

add_subdirectory(libraries)
add_subdirectory(source)
add_subdirectory(interfaces)
