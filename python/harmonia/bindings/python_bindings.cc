/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Headers for binding functions
/**************************************/
// The following comment block is used for
// gr_modtool to insert function prototypes
// Please do not delete
/**************************************/
// BINDING_FUNCTION_PROTOTYPES(
    void bind_pdu_complex_to_mag(py::module& m);
    void bind_pdu_fft(py::module& m);
    void bind_device(py::module& m);
    void bind_frequency_pk_est(py::module& m);
    void bind_single_tone_src(py::module& m);
    void bind_usrp_radar_2(py::module& m);
    void bind_SDR_tagger(py::module& m);
    void bind_usrp_radar_all(py::module& m);
    void bind_usrp_radar_tx(py::module& m);
    void bind_usrp_radar_rx(py::module& m);
    void bind_clock_drift_est(py::module& m);
    void bind_time_pk_est(py::module& m);
    void bind_buffer_corrector(py::module& m);
// ) END BINDING_FUNCTION_PROTOTYPES


// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(harmonia_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Allow access to base block methods
    py::module::import("gnuradio.gr");

    /**************************************/
    // The following comment block is used for
    // gr_modtool to insert binding function calls
    // Please do not delete
    /**************************************/
    // BINDING_FUNCTION_CALLS(
    bind_pdu_complex_to_mag(m);
    bind_pdu_fft(m);
    bind_device(m);
    bind_frequency_pk_est(m);
    bind_single_tone_src(m);
    bind_usrp_radar_2(m);
    bind_SDR_tagger(m);
    bind_usrp_radar_all(m);
    bind_usrp_radar_tx(m);
    bind_usrp_radar_rx(m);
    bind_clock_drift_est(m);
    bind_time_pk_est(m);
    bind_buffer_corrector(m);
    // ) END BINDING_FUNCTION_CALLS
}