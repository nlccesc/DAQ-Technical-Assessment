# Install script for directory: /Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/build/dbcppp/libdbcppp.3.8.0.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libdbcppp.3.8.0.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libdbcppp.3.8.0.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libdbcppp.3.8.0.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/build/dbcppp/libdbcppp.dylib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dbcppp" TYPE FILE FILES
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Attribute.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/AttributeDefinition.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/BitTiming.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/CApi.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/EnvironmentVariable.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Export.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Iterator.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Message.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Network.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Network2Functions.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Node.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/Signal.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/SignalGroup.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/SignalMultiplexerValue.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/SignalType.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/ValueEncodingDescription.h"
    "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/dbcppp/include/dbcppp/ValueTable.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/haohan/Desktop/DAQ-Technical-Assessment/firmware/solution/build/dbcppp/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
