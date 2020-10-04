add_executable(
  NFSmartPlayerTests
  NFSmartPlayerTestRunner.cpp
  ClientTest.cpp
  EdgeTest.cpp
  GraphTest.cpp
  nf_smart_player_test.cpp
  EdgeImplementationTest.cpp
  GraphImplementationTest.cpp
  CallbackTypesTest.cpp
  ClientImplementationTest.cpp
  ErrorCodeTest.cpp)
set(TEST_LINK_LIBS NFSmartPlayer ${Boost_LIBRARIES})
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  list(APPEND TEST_LINK_LIBS /usr/lib/x86_64-linux-gnu/libstdc++.so.6)
endif()
target_link_libraries(NFSmartPlayerTests ${TEST_LINK_LIBS})
target_include_directories(
  NFSmartPlayer
  PUBLIC
  ${NFSMARTPLAYER_SOURCE_DIRECTORY})
