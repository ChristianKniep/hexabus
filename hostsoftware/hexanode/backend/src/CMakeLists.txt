# -*- mode: cmake; -*-

file(GLOB_RECURSE h_files "*.hpp,*.h")
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
set(midi2hexabus_src "midi2hexabus.cpp")
set(hexabus_webbackend_src "hexabus_webbackend.cpp")
set(simulate_energyflows_src "simulate_energyflows.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}/libhexanode
  ${Boost_INCLUDE_DIRS}
  ${RTMIDI_INCLUDE_DIR}
)
#link_directories(
#  #  ${SQLITE3_STATIC_LIBRARY_DIRS}
#  )

add_executable(midi2hexabus ${midi2hexabus_src})
add_executable(hexabus_webbackend ${hexabus_webbackend_src})
add_executable(simulate_energyflows ${simulate_energyflows_src})

# Link the executables
target_link_libraries(midi2hexabus 
  hexanode
  pthread
  rtmidi
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(hexabus_webbackend
  hexanode
  pthread
  ${CPPNETLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

target_link_libraries(simulate_energyflows
  hexanode
  pthread
  ${CPPNETLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${HXB_LIBRARIES}
  ${Boost_LIBRARIES}
)

# add programs to the install target 
INSTALL(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/midi2hexabus
  ${CMAKE_CURRENT_BINARY_DIR}/hexabus_webbackend
  ${CMAKE_CURRENT_BINARY_DIR}/simulate_energyflows
  DESTINATION bin)
