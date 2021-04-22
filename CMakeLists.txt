cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

project(bw)

find_package(gflags REQUIRED)
find_package(TAPA REQUIRED)
find_package(FRT REQUIRED)

if(NOT PORT_WIDTH)
  set(PORT_WIDTH 64)
endif()

if(NOT CONCURRENCY)
  set(CONCURRENCY 4)
endif()

if(NOT PLATFORM)
  set(PLATFORM xilinx_u280_xdma_201920_3)
endif()

if(NOT PORT)
  set(PORT HBM[16])
endif()

add_library(bw-header INTERFACE)
target_compile_definitions(
  bw-header
  INTERFACE PORT_WIDTH=${PORT_WIDTH}
  INTERFACE CONCURRENCY=${CONCURRENCY})

add_library(bw-kernel)
target_sources(bw-kernel PRIVATE src/bw.cpp)
target_compile_features(bw-kernel PUBLIC cxx_std_11)
target_link_libraries(bw-kernel PUBLIC bw-header tapa::tapa)

add_executable(bw)
target_sources(bw PRIVATE src/bw-host.cpp)
target_compile_features(bw PRIVATE cxx_std_17)
target_link_libraries(bw PRIVATE bw-kernel tapa::tapa gflags)

add_tapa_target(
  bw-hw-xo
  INPUT src/bw.cpp
  TOP bw
  CFLAGS "-DPORT_WIDTH=${PORT_WIDTH} -DCONCURRENCY=${CONCURRENCY}"
  PLATFORM ${PLATFORM})

include(ProcessorCount)
processorcount(NPROC)
if(NPROC EQUAL 0)
  set(NPROC 8)
endif()

add_xocc_hw_link_targets(
  ${CMAKE_CURRENT_BINARY_DIR}
  --remote_ip_cache=$ENV{HOME}/.remote_ip_cache
  --sp=bw.mem:${PORT}
  --dk=chipscope:bw:m_axi_mem
  --vivado.synth.jobs=${NPROC}
  --vivado.impl.jobs=${NPROC}
  --vivado.prop=run.impl_1.STEPS.OPT_DESIGN.TCL.PRE=${CMAKE_CURRENT_SOURCE_DIR}/constraint.tcl
  INPUT bw-hw-xo
  HW_EMU_XCLBIN hw_emu_xclbin_target
  HW_XCLBIN hw_xclbin_target)

add_custom_target(
  bw-cosim
  COMMAND $<TARGET_FILE:bw>
          --bitstream=$<TARGET_PROPERTY:${hw_emu_xclbin_target},FILE_NAME>
  DEPENDS bw ${hw_emu_xclbin_target}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(
  bw-hw
  COMMAND $<TARGET_FILE:bw>
          --bitstream=$<TARGET_PROPERTY:${hw_xclbin_target},FILE_NAME> -logn=18
  DEPENDS bw ${hw_xclbin_target}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
