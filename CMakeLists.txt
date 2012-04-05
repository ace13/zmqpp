CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(zmqpp)

SET(ZMQPP_VERSION_MAJOR    2)
SET(ZMQPP_VERSION_MINOR    0)
SET(ZMQPP_VERSION_REVISION 0)

CONFIGURE_FILE(src/zmqpp/defines.hpp.in defines.hpp)

FIND_PACKAGE(Boost COMPONENTS program_options unit_test_framework)

FIND_LIBRARY(ZMQ_LIBRARY zmq)
FIND_PATH(ZMQ_INCLUDE_DIR zmq.h)

INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIR}
  ${ZMQ_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

SET(ZMQPP_INCLUDES
  ${CMAKE_CURRENT_BINARY_DIR}/defines.hpp
  src/zmqpp/compatibility.hpp
  src/zmqpp/context.hpp
  src/zmqpp/exception.hpp
  src/zmqpp/inet.hpp
  src/zmqpp/message.hpp
  src/zmqpp/poller.hpp
  src/zmqpp/socket.hpp
  src/zmqpp/socket_options.hpp
  src/zmqpp/socket_types.hpp
  src/zmqpp/zmqpp.hpp
)

SET(ZMQPP_SOURCE
  src/zmqpp/message.cpp
  src/zmqpp/poller.cpp
  src/zmqpp/socket.cpp
  src/zmqpp/zmqpp.cpp
)

ADD_LIBRARY(libzmqpp SHARED ${ZMQPP_INCLUDES} ${ZMQPP_SOURCE})
SET_TARGET_PROPERTIES(libzmqpp PROPERTIES PREFIX "" VERSION ${ZMQPP_VERSION_MAJOR}.${ZMQPP_VERSION_MINOR}.${ZMQPP_VERSION_REVISION}) 

SET(ZMQPP_CLIENT
  src/client/main.cpp
)

ADD_EXECUTABLE(zmqpp ${ZMQPP_CLIENT})

SET(ZMQPP_TESTS
  src/tests/test_context.cpp
  src/tests/test_inet.cpp
  src/tests/test_load.cpp
  src/tests/test_message.cpp
  src/tests/test_message_stream.cpp
  src/tests/test_poller.cpp
  src/tests/test_sanity.cpp
  src/tests/test_socket.cpp
  src/tests/test_socket_options.cpp
)

ADD_EXECUTABLE(zmqpp-tests ${ZMQPP_TESTS})

ADD_DEFINITIONS(-std=c++0x)

ADD_DEPENDENCIES(zmqpp libzmqpp)
ADD_DEPENDENCIES(zmqpp-tests libzmqpp)

TARGET_LINK_LIBRARIES(zmqpp ${ZMQ_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} libzmqpp)
TARGET_LINK_LIBRARIES(zmqpp-tests ${ZMQ_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} libzmqpp)

ADD_TEST(TestSuite zmqpp-tests)

INSTALL(TARGETS libzmqpp zmqpp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)
INSTALL(FILES ${ZMQPP_INCLUDES} DESTINATION include/zmqpp)
