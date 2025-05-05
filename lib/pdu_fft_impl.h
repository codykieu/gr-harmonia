/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_PDU_FFT_IMPL_H
#define INCLUDED_HARMONIA_PDU_FFT_IMPL_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/harmonia/pdu_fft.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <plasma_dsp/fft.h>

namespace gr {
namespace harmonia {

class pdu_fft_impl : public pdu_fft
{
private:
    size_t d_fftsize;
    size_t d_queue_depth;

    void handle_msg(pmt::pmt_t msg);

    pmt::pmt_t d_out_port;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_data;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_fft_size_key;

    af::Backend d_backend;

public:
    pdu_fft_impl(size_t nfft);
    ~pdu_fft_impl();

    void set_metadata_keys(const std::string& fft_size_key);
    void set_msg_queue_depth(size_t) override;
    void set_backend(Device::Backend) override;
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_PDU_FFT_IMPL_H */
