# Install script for directory: /home/cody/gr-harmonia/grc

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/gnuradio/grc/blocks" TYPE FILE FILES
    "/home/cody/gr-harmonia/grc/harmonia_pdu_complex_to_mag.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_pdu_fft.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_frequency_pk_est.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_single_tone_src.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_usrp_radar_2.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_SDR_tagger.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_usrp_radar_all.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_usrp_radar_tx.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_usrp_radar_rx.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_clock_drift_est.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_time_pk_est.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_buffer_corrector.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_clockbias_phase_est.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_QPSK_mod.block.yml"
    "/home/cody/gr-harmonia/grc/harmonia_QPSK_demod.block.yml"
    )
endif()

