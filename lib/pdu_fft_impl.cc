/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_fft_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>

namespace gr {
namespace harmonia {

pdu_fft::sptr pdu_fft::make(size_t nfft) 
{ 
    return gnuradio::make_block_sptr<pdu_fft_impl>(nfft); 
}


/*
 * The private constructor
 */
pdu_fft_impl::pdu_fft_impl(size_t nfft)
    : gr::block("pdu_fft",
                gr::io_signature::make(0,0,0),
                gr::io_signature::make(0,0,0)),
    d_fftsize(nfft)
{
    d_in_port = PMT_HARMONIA_IN;
    d_out_port = PMT_HARMONIA_OUT;
    d_meta = pmt::make_dict();
    d_data = pmt::make_c32vector(0, 0);
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

/*
 * Our virtual destructor.
 */
pdu_fft_impl::~pdu_fft_impl() {}

void pdu_fft_impl::handle_msg(pmt::pmt_t msg)
{
    if (this->nmsgs(d_in_port) > d_queue_depth) {
        return;
    }

    // Read the input PDU
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, meta);

    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }

    // Retrieves length of samples
    size_t n = pmt::length(samples);
    const std::complex<float>* in_data = pmt::c32vector_elements(samples,n);

    // Calculating FFT of input data
    af::array af_input(n, reinterpret_cast<const af::cfloat*>(in_data));
    af::array af_fft = af::fft(af_input);

    size_t io(0);
    d_data = pmt::make_c32vector(n, 0);
    gr_complex* out = pmt::c32vector_writable_elements(d_data, io);
    af_fft.host(out);

    // Send the data as a message
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
    // Reset the metadata output
    d_meta = pmt::make_dict();
}

void pdu_fft_impl::set_metadata_keys(const std::string& fft_size_key)
{
    d_fft_size_key = pmt::intern(fft_size_key);

    d_meta = pmt::dict_add(d_meta, d_fft_size_key, pmt::from_long(d_fftsize));
}

void pdu_fft_impl::set_msg_queue_depth(size_t depth) { d_queue_depth = depth; }

void pdu_fft_impl::set_backend(Device::Backend backend)
{
    switch (backend) {
    case Device::CPU:
        d_backend = AF_BACKEND_CPU;
        break;
    case Device::CUDA:
        d_backend = AF_BACKEND_CUDA;
        break;
    case Device::OPENCL:
        d_backend = AF_BACKEND_OPENCL;
        break;
    default:
        d_backend = AF_BACKEND_DEFAULT;
        break;
    }
    af::setBackend(d_backend);
}

} /* namespace harmonia */
} /* namespace gr */
