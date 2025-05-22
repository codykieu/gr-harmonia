# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT harmonia_sources)
    message(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT harmonia_sources)

########################################################################
# Check for pygccxml
########################################################################
gr_python_check_module_raw("pygccxml" "import pygccxml" PYGCCXML_FOUND)

include(GrPybind)

########################################################################
# Python Bindings
########################################################################
list(APPEND harmonia_python_files
    pdu_complex_to_mag_python.cc 
    pdu_fft_python.cc
    device_python.cc
    frequency_pk_est_python.cc
    single_tone_src_python.cc
    usrp_radar_2_python.cc
    SDR_tagger_python.cc
    usrp_radar_all_python.cc
    usrp_radar_tx_python.cc
    usrp_radar_rx_python.cc
    clock_drift_est_python.cc
    time_pk_est_python.cc
    buffer_corrector_python.cc
    clockbias_phase_est_python.cc
    QPSK_mod_python.cc
    QPSK_demod_python.cc
    python_bindings.cc
    )

gr_pybind_make_oot(harmonia ../../.. gr::harmonia "${harmonia_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(
    TARGET harmonia_python
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:harmonia_python>
            ${PROJECT_BINARY_DIR}/test_modules/gnuradio/harmonia/)

install(
    TARGETS harmonia_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/harmonia
    COMPONENT pythonapi)
