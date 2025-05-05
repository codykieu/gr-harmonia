# Install script for directory: /home/cody/gr-harmonia/include/gnuradio/harmonia

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
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/gnuradio/harmonia" TYPE FILE FILES
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/api.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/pdu_complex_to_mag.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/pmt_constants.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/pdu_fft.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/device.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/frequency_pk_est.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/single_tone_src.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/usrp_radar_2.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/SDR_tagger.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/usrp_radar_all.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/usrp_radar_tx.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/usrp_radar_rx.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/clock_drift_est.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/time_pk_est.h"
    "/home/cody/gr-harmonia/include/gnuradio/harmonia/buffer_corrector.h"
    )
endif()

