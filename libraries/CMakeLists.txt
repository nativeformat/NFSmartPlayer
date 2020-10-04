# Add duktape
add_library(duktape STATIC ${CMAKE_CURRENT_SOURCE_DIR}/duktape/src/duk_config.h
                           ${CMAKE_CURRENT_SOURCE_DIR}/duktape/src/duktape.h
                           ${CMAKE_CURRENT_SOURCE_DIR}/duktape/src/duktape.c)

# Add boost
set(BOOST_MAJOR 1)
set(BOOST_MINOR 64)
set(BOOST_PATCH 0)
set(BOOST_VER_DOT "${BOOST_MAJOR}.${BOOST_MINOR}.${BOOST_PATCH}")
set(BOOST_VER_UNDER "${BOOST_MAJOR}_${BOOST_MINOR}_${BOOST_PATCH}")

message(STATUS "Finding Boost via find_package")
find_package(Boost ${BOOST_VER_DOT})
if (NOT Boost_FOUND)

  set(BOOST_EXTRACTION_FOLDER_STRIPPED ${CMAKE_CURRENT_SOURCE_DIR})
  set(BOOST_EXTRACTION_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/boost_${BOOST_VER_UNDER}")
  set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/boost_${BOOST_VER_UNDER}")

  message(STATUS "Finding Boost via Download")

  if (NOT EXISTS "${BOOST_EXTRACTION_FOLDER}")
    message(STATUS "Downloading Boost ${BOOST_VER_DOT}")
    execute_process(COMMAND wget http://sourceforge.net/projects/boost/files/boost/${BOOST_VER_DOT}/boost_${BOOST_VER_UNDER}.tar.gz -O ${CMAKE_CURRENT_SOURCE_DIR}/boost_${BOOST_VER_UNDER}.tar.gz RESULT_VARIABLE retcode)
    if(NOT "${retcode}" STREQUAL "0")
      message( FATAL_ERROR "Could not download Boost")
    endif()
    execute_process(COMMAND tar -xzf ${CMAKE_CURRENT_SOURCE_DIR}/boost_${BOOST_VER_UNDER}.tar.gz -C ${CMAKE_CURRENT_SOURCE_DIR} RESULT_VARIABLE ret)
    if(NOT "${retcode}" STREQUAL "0")
      message( FATAL_ERROR "Could not extract Boost")
    endif()
  endif()

  set(BOOST_LIB_DIR "${BOOST_EXTRACTION_FOLDER_STRIPPED}/boost_${BOOST_VER_UNDER}/libs")
  set(BOOST_GRAPH_SRC_DIR "${BOOST_LIB_DIR}/graph/src")
  set(BOOST_PROGRAM_OPTIONS_SRC_DIR "${BOOST_LIB_DIR}/program_options/src")
  set(BOOST_THREAD_SRC_DIR "${BOOST_LIB_DIR}/thread/src")
  set(BOOST_CHRONO_SRC_DIR "${BOOST_LIB_DIR}/chrono/src")
  set(BOOST_SYSTEM_SRC_DIR "${BOOST_LIB_DIR}/system/src")
  set(BOOST_TEST_SRC_DIR "${BOOST_LIB_DIR}/test/src")
  add_library(boost_graph STATIC ${BOOST_GRAPH_SRC_DIR}/graphml.cpp
                                ${BOOST_GRAPH_SRC_DIR}/read_graphviz_new.cpp)
  add_library(boost_program_options STATIC ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/cmdline.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/config_file.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/convert.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/options_description.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/parsers.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/positional_options.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/split.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/utf8_codecvt_facet.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/value_semantic.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/variables_map.cpp
                                          ${BOOST_PROGRAM_OPTIONS_SRC_DIR}/winmain.cpp)
  add_library(boost_thread STATIC ${BOOST_THREAD_SRC_DIR}/future.cpp
                                  ${BOOST_THREAD_SRC_DIR}/tss_null.cpp
                                  ${BOOST_THREAD_SRC_DIR}/pthread/once_atomic.cpp
                                  ${BOOST_THREAD_SRC_DIR}/pthread/once.cpp
                                  ${BOOST_THREAD_SRC_DIR}/pthread/thread.cpp
                                  #${BOOST_THREAD_SRC_DIR}/win32/tss_dll.cpp
                                  #${BOOST_THREAD_SRC_DIR}/win32/tss_pe.cpp
                                  #${BOOST_THREAD_SRC_DIR}/win32/thread.cpp
                                  )
  add_library(boost_chrono STATIC ${BOOST_CHRONO_SRC_DIR}/chrono.cpp
                                  ${BOOST_CHRONO_SRC_DIR}/process_cpu_clocks.cpp
                                  ${BOOST_CHRONO_SRC_DIR}/thread_clock.cpp)
  add_library(boost_system STATIC ${BOOST_SYSTEM_SRC_DIR}/error_code.cpp)
  add_library(boost_test STATIC ${BOOST_TEST_SRC_DIR}/compiler_log_formatter.cpp
                                #${BOOST_TEST_SRC_DIR}/cpp_main.cpp
                                ${BOOST_TEST_SRC_DIR}/debug.cpp
                                ${BOOST_TEST_SRC_DIR}/decorator.cpp
                                ${BOOST_TEST_SRC_DIR}/execution_monitor.cpp
                                ${BOOST_TEST_SRC_DIR}/framework.cpp
                                ${BOOST_TEST_SRC_DIR}/junit_log_formatter.cpp
                                ${BOOST_TEST_SRC_DIR}/plain_report_formatter.cpp
                                ${BOOST_TEST_SRC_DIR}/progress_monitor.cpp
                                ${BOOST_TEST_SRC_DIR}/results_collector.cpp
                                ${BOOST_TEST_SRC_DIR}/results_reporter.cpp
                                ${BOOST_TEST_SRC_DIR}/test_main.cpp
                                ${BOOST_TEST_SRC_DIR}/test_tools.cpp
                                ${BOOST_TEST_SRC_DIR}/test_tree.cpp
                                ${BOOST_TEST_SRC_DIR}/unit_test_log.cpp
                                ${BOOST_TEST_SRC_DIR}/unit_test_main.cpp
                                ${BOOST_TEST_SRC_DIR}/unit_test_monitor.cpp
                                ${BOOST_TEST_SRC_DIR}/unit_test_parameters.cpp
                                ${BOOST_TEST_SRC_DIR}/xml_log_formatter.cpp
                                ${BOOST_TEST_SRC_DIR}/xml_report_formatter.cpp)
  set(Boost_GRAPH_LIBRARY boost_graph)
  set(Boost_PROGRAM_OPTIONS_LIBRARY boost_program_options)
  set(Boost_THREAD_LIBRARY boost_thread)
  set(Boost_CHRONO_LIBRARY boost_chrono)
  set(Boost_SYSTEM_LIBRARY boost_system)
  set(Boost_TEST_LIBRARY boost_test)
  set(Boost_INCLUDE_DIR "${BOOST_EXTRACTION_FOLDER_STRIPPED}/boost_1_64_0")
  set(Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR} PARENT_SCOPE)
  target_include_directories(boost_graph PUBLIC ${Boost_INCLUDE_DIR})
  target_include_directories(boost_program_options PUBLIC ${Boost_INCLUDE_DIR})
  target_include_directories(boost_thread PUBLIC ${Boost_INCLUDE_DIR})
  target_include_directories(boost_chrono PUBLIC ${Boost_INCLUDE_DIR})
  target_include_directories(boost_system PUBLIC ${Boost_INCLUDE_DIR})
  target_include_directories(boost_test PUBLIC ${Boost_INCLUDE_DIR})

  # Boost_LIBRARIES must be populated from the project root and be a valid
  # argument to target_link_libraries() for the plugins and CLI
  set(Boost_LIBRARIES ${BOOST_GRAPH_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_TEST_LIBRARY})
  set(Boost_LIBRARIES ${BOOST_GRAPH_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_TEST_LIBRARY}
    PARENT_SCOPE)
  set(Boost_FOUND 1)
  set(Boost_FOUND 1 PARENT_SCOPE)
endif()

set(BUILD_SHARED_LIBS OFF)
set(BUILD_SAMPLES OFF)
set(BUILD_TESTS OFF)
add_subdirectory(NFDecoder)
add_subdirectory(NFDriver)
add_subdirectory(NFLogger)
add_subdirectory(NFGrapher)
add_subdirectory(oscpack)
add_subdirectory(PeqBank)

# Fix some OSCPack headers
set(OSCPACK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/oscpack)
target_include_directories(oscpack PUBLIC ${OSCPACK_ROOT_DIR})

message(STATUS "NFSmartPlayer libraries: BOOST Info")
message(STATUS "NFSmartPlayer libraries: Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR}")
message(STATUS "NFSmartPlayer libraries: Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")
message(STATUS "NFSmartPlayer libraries: Boost_LIBRARIES ${Boost_LIBRARIES}")
message(STATUS "NFSmartPlayer libraries: Boost_FOUND ${Boost_FOUND}")
message(STATUS "NFSmartPlayer libraries: BOOST_ROOT ${BOOST_ROOT}")