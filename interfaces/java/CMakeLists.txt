# Generate Java interfaces
message("Generating JNI Interface")

if(ANDROID)
  # https://stackoverflow.com/a/51764145
  set(JAVA_AWT_LIBRARY NotNeeded)
  set(JAVA_JVM_LIBRARY NotNeeded)
  set(JAVA_INCLUDE_PATH2 NotNeeded)
  set(JAVA_AWT_INCLUDE_PATH NotNeeded)
endif()

find_package(Java REQUIRED COMPONENTS Runtime Development)
find_package(JNI REQUIRED)
execute_process(COMMAND
  ${Java_JAVAC_EXECUTABLE} -h . com/spotify/nativeformat/Player.java
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE JAVAC_CODE)
if(NOT JAVAC_CODE STREQUAL "0")
  message(FATAL_ERROR "JNI Generation Error: Exited with code ${JAVAC_CODE}")
endif()
add_library(NFSmartPlayerJava
  SHARED
  Player.cpp)
target_link_libraries(NFSmartPlayerJava
  NFSmartPlayer
  ${JNI_LIBRARIES})
target_include_directories(NFSmartPlayerJava
  PRIVATE
  ${JNI_INCLUDE_DIRS})

message("End of Generating JNI Interface")