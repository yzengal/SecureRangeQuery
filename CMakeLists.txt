cmake_minimum_required(VERSION 3.8)

project(ICDE18 C CXX)
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(BOOST_ROOT "/home/yzengal/software/boost_1_85_0")  
find_package(Boost 1.85.0 REQUIRED)  

if(NOT Boost_FOUND)  
    message(FATAL_ERROR "Could not find Boost version 1.85.0 or higher")  
endif()  

include_directories("${Boost_INCLUDE_DIR}") 

include(./common.cmake)

get_filename_component(icde_proto "./proto/ICDE18.proto" ABSOLUTE)
get_filename_component(icde_proto_path "${icde_proto}" PATH)
#set(CMAKE_EXE_LINKER_FLAGS "-static") ## enable this, program has segmentation fault

#compile proto file
set(icde_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/ICDE18.pb.cc")
set(icde_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ICDE18.pb.h")
set(icde_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/ICDE18.grpc.pb.cc")
set(icde_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ICDE18.grpc.pb.h")

add_custom_command(
      OUTPUT "${icde_proto_srcs}" "${icde_proto_hdrs}" "${icde_grpc_srcs}" "${icde_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${icde_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${icde_proto}"
      DEPENDS "${icde_proto}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_library(grpc_proto
  ${icde_grpc_srcs}
  ${icde_grpc_hdrs}
  ${icde_proto_srcs}
  ${icde_proto_hdrs}
  )

target_link_libraries(grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(differentialprivacy
  "./cpp/differentialprivacy.h"
  "./cpp/differentialprivacy.cpp")

target_link_libraries(differentialprivacy
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(AES
  "./cpp/AES.h"
  "./cpp/AES.cpp")

target_link_libraries(AES
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(global
  "./cpp/global.h"
  "./cpp/global.cpp")

target_link_libraries(global
  grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(grid
  "./cpp/grid.hpp")

target_link_libraries(grid
  grpc_proto
  global
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

foreach(_target
  silo server)
  add_executable(${_target} "./cpp/${_target}.cpp")
  target_link_libraries(${_target}
  grpc_proto
  differentialprivacy
  AES
  global
  grid
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
