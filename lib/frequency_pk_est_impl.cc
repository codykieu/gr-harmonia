/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "frequency_pk_est_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <math.h>

namespace gr {
namespace harmonia {

frequency_pk_est::sptr frequency_pk_est::make(size_t nfft, double pulse_width, double cap_length,
    double samp_rate, double NLLS_iter, double NLLS_pts)
{
    return gnuradio::make_block_sptr<frequency_pk_est_impl>(nfft, pulse_width, cap_length, 
        samp_rate, NLLS_iter, NLLS_pts);
}


/*
 * The private constructor
 */
frequency_pk_est_impl::frequency_pk_est_impl(size_t nfft, double pulse_width, double cap_length,
    double samp_rate, double NLLS_iter, double NLLS_pts)
    : gr::block("frequency_pk_est",
                gr::io_signature::make(0,0,0),
                gr::io_signature::make(0,0,0)),
    d_fftsize(nfft),
    d_pulse_width(pulse_width),
    d_cap_length(cap_length),
    d_samp_rate(samp_rate),
    d_NLLS_iter(NLLS_iter),
    d_NLLS_pts(NLLS_pts)
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
frequency_pk_est_impl::~frequency_pk_est_impl() {}

void frequency_pk_est_impl::handle_msg(pmt::pmt_t msg)
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
    GR_LOG_INFO(d_logger, "Received PDU with length: " + std::to_string(n));
    const std::complex<float>* in_data = pmt::c32vector_elements(samples,n);

    // Calculating absolute FFT of input data
    af::array af_input(n, reinterpret_cast<const af::cfloat*>(in_data));
    //af::array af_fft = af::abs(af::fftNorm(af_input, 1.0, d_fftsize));
    af::array af_fft = af::abs(af::fft(af_input));
    af_fft = ::plasma::fftshift(af_fft, 0);
    
    // Outputting max of data
    float max_fft;
    unsigned max_idx;
    af::max(&max_fft, &max_idx, af_fft);
    
    // Generate frequency axis
    af::array f_axis = (-d_samp_rate / 2.0) + (af::seq(0, d_fftsize*n - 1) * (d_samp_rate / (d_fftsize*n)));
    

    // -------------------------------------------
    std::vector<float> f_axis_host(f_axis.elements());  // Ensure correct data type (float/double)
    f_axis.host(f_axis_host.data());  // Copy data to CPU memory
    
    // Extract first and last values correctly
    double first_value = static_cast<double>(f_axis_host[0]);
    double last_value = static_cast<double>(f_axis_host[f_axis_host.size() - 1]);
    
    // Log values
    GR_LOG_INFO(d_logger, "First Frequency Value: " + std::to_string(first_value) +
                          ", Last Frequency Value: " + std::to_string(last_value));
    // -------------------------------------------

    size_t f_axis_n = f_axis.elements();
    GR_LOG_INFO(d_logger, "Frequency Axis Length: " + std::to_string(f_axis_n));

    // Ensure max_idx is valid
    if (max_idx >= 0 && max_idx < d_fftsize*n) {
        double value;
        af::array value_array = f_axis(max_idx); // Get indexed value as an af::array

        if (value_array.elements() == 1) {  // Ensure it's a single value
            value_array.host(&value);  // Extract scalar safely

            // Log the max FFT value and its corresponding frequency
            GR_LOG_INFO(d_logger, "Frequency: " + std::to_string(value)
                        + " Max Index: " + std::to_string(max_idx));
        } else {
            GR_LOG_WARN(d_logger, "Indexing returned multiple elements!");
        }
    } else {
        GR_LOG_WARN(d_logger, "Invalid max_idx: " + std::to_string(max_idx));
    }

    // Outputting data
    size_t io(0);
    d_data = pmt::make_c32vector(n, 0);
    gr_complex* out = pmt::c32vector_writable_elements(d_data, io);
    af_fft.host(out);

    // Send the data as a message
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
    // Reset the metadata output
    d_meta = pmt::make_dict();

    //NLLS
    double lambda_buffer[] = {af::abs(af_fft(max_idx)), 0, (d_pulse_width/d_cap_length)};
    af::array lambda(3, lambda_buffer);
    GR_LOG_INFO(d_logger, "Lambda: " + std::to_string(lambda));
}

void frequency_pk_est_impl::set_metadata_keys(const std::string& fft_size_key)
{
    d_fft_size_key = pmt::intern(fft_size_key);

    d_meta = pmt::dict_add(d_meta, d_fft_size_key, pmt::from_long(d_fftsize));
}

void frequency_pk_est_impl::set_msg_queue_depth(size_t depth) { d_queue_depth = depth; }

void frequency_pk_est_impl::set_backend(Device::Backend backend)
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
