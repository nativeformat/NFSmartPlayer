message(STATUS "NFSmartPlayer source/cli: BOOST Info")
message(STATUS
  "NFSmartPlayer source/cli: Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR}")
message(STATUS
  "NFSmartPlayer source/cli: Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")
message(STATUS
  "NFSmartPlayer source/cli: Boost_LIBRARIES ${Boost_LIBRARIES}")
message(STATUS "NFSmartPlayer source/cli: Boost_FOUND ${Boost_FOUND}")
message(STATUS "NFSmartPlayer source/cli: BOOST_ROOT ${BOOST_ROOT}")

# Add the player CLI
add_executable(NFSmartPlayerCLI main.cpp)
target_compile_definitions(NFSmartPlayerCLI PUBLIC -DNF_LOG_ERROR=1)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(NFSmartPlayerCLI NFSmartPlayer)
else()
  target_link_libraries(NFSmartPlayerCLI
    NFSmartPlayer
    /usr/lib/x86_64-linux-gnu/libstdc++.so.6)
endif()

install(TARGETS NFSmartPlayerCLI DESTINATION /usr/bin)

