set(NFSMARTPLAYEROBJC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(
  NFSmartPlayerObjC
  "${NFSMARTPLAYEROBJC_INCLUDE_DIR}/NFSmartPlayerObjC/NFSmartPlayerObjC.h"
  "${NFSMARTPLAYEROBJC_INCLUDE_DIR}/NFSmartPlayerObjC/NFSmartPlayerGraph.h"
  "${NFSMARTPLAYEROBJC_INCLUDE_DIR}/NFSmartPlayerObjC/NFSmartPlayerScript.h"
  "${NFSMARTPLAYEROBJC_INCLUDE_DIR}/NFSmartPlayerObjC/NFSmartPlayerParam.h"
  "${NFSMARTPLAYEROBJC_INCLUDE_DIR}/NFSmartPlayerObjC/NFSmartPlayerNode.h"
  "${NFSMARTPLAYEROBJC_INCLUDE_DIR}/NFSmartPlayerObjC/NFSmartPlayerEdge.h"
  NFSmartPlayerObjC.m
  NFSmartPlayerGraph.m
  NFSmartPlayerGraph+Private.h
  NFSmartPlayerScript+Private.h
  NFSmartPlayerScript.m
  NFSmartPlayerParam+Private.h
  NFSmartPlayerParam.m
  NFSmartPlayerNode.m
  NFSmartPlayerNode+Private.h
  NFSmartPlayerEdge.m
  NFSmartPlayerEdge+Private.h)
find_library(FOUNDATION Foundation)
find_library(CORE_SERVICES CoreServices)
find_library(ACCELERATE Accelerate)
target_link_libraries(
  NFSmartPlayerObjC
  NFSmartPlayer
  ${FOUNDATION}
  ${CORE_SERVICES}
  ${ACCELERATE}
  ${ELASTIQUE_LIBRARIES})
set_property(
  TARGET
  NFSmartPlayerObjC
  APPEND_STRING
  PROPERTY
  COMPILE_FLAGS "-fobjc-arc")
target_include_directories(
  NFSmartPlayerObjC
  PUBLIC
  ${NFSMARTPLAYEROBJC_INCLUDE_DIR})

cmake_policy(SET CMP0026 OLD)

# BEGIN PRE-LINK LOGIC

# Recursively find all dependencies
function(_find_deps)
  get_target_property(DEPS ${ARGV0} INTERFACE_LINK_LIBRARIES)
  if(DEPS STREQUAL "DEPS-NOTFOUND")
    return()
  endif()
  foreach(DEPENDENCY ${DEPS})
    string(REPLACE "$<LINK_ONLY:" "" DEPENDENCY ${DEPENDENCY})
    string(REPLACE ">" "" DEPENDENCY ${DEPENDENCY})

    get_target_property(CURRENT_DEPS NFSmartPlayerObjC ALL_DEPS)
    if(NOT "${DEPENDENCY}" IN_LIST CURRENT_DEPS)
      set_property(
        TARGET
        NFSmartPlayerObjC
        APPEND
        PROPERTY
        ALL_DEPS ${DEPENDENCY})
      if(TARGET "${DEPENDENCY}")
        _find_deps("${DEPENDENCY}")
      endif()
    endif()
  endforeach()
endfunction()

_find_deps(NFSmartPlayerObjC)
get_target_property(ALL_DEPS NFSmartPlayerObjC ALL_DEPS)
list(SORT ALL_DEPS)

# Finds the actual static library location for each dependency
set(FILTERED_DEPS ${ELASTIQUE_LIBRARIES})
foreach(DEPENDENCY ${ALL_DEPS})

  if(NOT TARGET ${DEPENDENCY})
    continue()
  endif()

  get_target_property(ALIASED ${DEPENDENCY} ALIASED_TARGET)
  if(ALIASED)
    set(DEPENDENCY ${ALIASED})
  endif()
  if(DEPENDENCY MATCHES "^/.*\\.a$")
    list(APPEND FILTERED_DEPS ${DEPENDENCY})
  elseif(TARGET ${DEPENDENCY})
    get_target_property(DEPENDENCY_TYPE ${DEPENDENCY} TYPE)
    if("${DEPENDENCY_TYPE}" STREQUAL "INTERFACE_LIBRARY")
      continue()
    endif()
    get_target_property(
      DEPENDENCY_ARCHIVE_OUTPUT_DIRECTORY
      ${DEPENDENCY}
      ARCHIVE_OUTPUT_DIRECTORY)
    get_target_property(
      DEPENDENCY_IMPORTED_LOCATION
      ${DEPENDENCY}
      IMPORTED_LOCATION)
    get_target_property(DEPENDENCY_OUTPUT_NAME ${DEPENDENCY} OUTPUT_NAME)
    get_target_property(DEPENDENCY_OUTPUT_POSTFIX ${DEPENDENCY} DEBUG_POSTFIX)
    get_target_property(DEPENDENCY_LOCATION ${DEPENDENCY} LOCATION)
    if("${DEPENDENCY_OUTPUT_NAME}" MATCHES "DEPENDENCY_OUTPUT_NAME-NOTFOUND")
      set(DEPENDENCY_OUTPUT_NAME "${DEPENDENCY}")
    endif()
    if("${DEPENDENCY_OUTPUT_POSTFIX}"
      MATCHES
      "DEPENDENCY_OUTPUT_POSTFIX-NOTFOUND" OR CREATE_RELEASE_BUILD)
      set(DEPENDENCY_OUTPUT_POSTFIX "")
    endif()
    # Fix path to account for DEPENDENCY_OUTPUT_POSTFIX
    string(REPLACE
      "lib${DEPENDENCY_OUTPUT_NAME}"
      "lib${DEPENDENCY_OUTPUT_NAME}${DEPENDENCY_OUTPUT_POSTFIX}"
      DEPENDENCY_LOCATION
      ${DEPENDENCY_LOCATION})
    set(
      DEPENDENCY_OUTPUT_NAME
      "lib${DEPENDENCY_OUTPUT_NAME}${DEPENDENCY_OUTPUT_POSTFIX}.a")
    if(DEPENDENCY_IMPORTED_LOCATION)
      set(DEPENDENCY_LOCATION ${DEPENDENCY_IMPORTED_LOCATION})
    elseif(DEPENDENCY_ARCHIVE_OUTPUT_DIRECTORY)
      set(DEPENDENCY_LOCATION
        "${DEPENDENCY_ARCHIVE_OUTPUT_DIRECTORY}\
/$(CONFIGURATION)/${DEPENDENCY_OUTPUT_NAME}")
    elseif("${DEPENDENCY_LOCATION}" MATCHES "DEPENDENCY_LOCATION-NOTFOUND")
      set(DEPENDENCY_LOCATION "$(BUILT_PRODUCTS_DIR)/${DEPENDENCY_OUTPUT_NAME}")
    endif()
    if(NOT DEPENDENCY_LOCATION MATCHES "\\.a$")
      continue()
    endif()
    string(TOLOWER "${DEPENDENCY}" DEPENDENCY_LOWER)
    if("${DEPENDENCY_LOWER}" MATCHES "objc")
      list(APPEND FILTERED_DEPS "-force_load")
    endif()
    list(APPEND FILTERED_DEPS ${DEPENDENCY_LOCATION})
  endif()
endforeach()

set_target_properties(
  NFSmartPlayerObjC
  PROPERTIES
  XCODE_ATTRIBUTE_GENERATE_MASTER_OBJECT_FILE "YES")
set_target_properties(
  NFSmartPlayerObjC
  PROPERTIES
  XCODE_ATTRIBUTE_PRELINK_LIBS "$<JOIN:${FILTERED_DEPS}, >")
set_target_properties(
  NFSmartPlayerObjC
  PROPERTIES
  XCODE_ATTRIBUTE_PRELINK_FLAGS "-ObjC")
add_custom_command(
  TARGET
  NFSmartPlayerObjC
  PRE_BUILD
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check-lib-pre.sh
  ${FILTERED_DEPS})
add_subdirectory(tests)
