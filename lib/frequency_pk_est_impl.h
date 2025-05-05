/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_FREQUENCY_PK_EST_IMPL_H
#define INCLUDED_HARMONIA_FREQUENCY_PK_EST_IMPL_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/harmonia/frequency_pk_est.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <plasma_dsp/fft.h>

namespace gr {
namespace harmonia {

class frequency_pk_est_impl : public frequency_pk_est
{
private:
    // Parameters
    size_t d_fftsize;
    double d_pulse_width;
    double d_cap_length;
    double d_samp_rate;
    double d_NLLS_iter;
    double d_NLLS_pts;
    bool d_enable_out;

    // Variables
    double f_est;
    size_t d_queue_depth;

    // Message Ports    
    pmt::pmt_t d_out_port;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_f_out_port;

    // Waveform object and IQ data
    pmt::pmt_t d_data;

    // Metadata 
    pmt::pmt_t d_meta;
    pmt::pmt_t d_meta_f;
    pmt::pmt_t d_freq_est_key; 
    pmt::pmt_t sdr1_dict;
    pmt::pmt_t sdr2_dict;
    pmt::pmt_t sdr3_dict;

    void handle_msg(pmt::pmt_t msg);

    af::Backend d_backend;

public:
    frequency_pk_est_impl(size_t nfft, double pulse_width, double cap_length,
        double samp_rate, double NLLS_iter, double NLLS_pts, bool enable_out);
    ~frequency_pk_est_impl();

    void init_metadata_keys();
    void set_msg_queue_depth(size_t) override;
    void set_backend(Device::Backend) override;
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_FREQUENCY_PK_EST_IMPL_H */
