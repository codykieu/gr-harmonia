/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_complex_to_mag_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace harmonia {

// Factory Function (Creates the Block)
pdu_complex_to_mag::sptr pdu_complex_to_mag::make()
{
    return gnuradio::make_block_sptr<pdu_complex_to_mag_impl>();
}

/*
 * The private constructor (Initializes the Block)
 */
pdu_complex_to_mag_impl::pdu_complex_to_mag_impl()
    : gr::block("pdu_complex_to_mag",
                gr::io_signature::make(0,0,0),
                gr::io_signature::make(0,0,0)),
    in_port(PMT_HARMONIA_IN),
    out_port(PMT_HARMONIA_OUT)
{
    // Registers Message Ports
    message_port_register_in(in_port);
    message_port_register_out(out_port);
    set_msg_handler(in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

/*
 * Our virtual destructor.
 */
pdu_complex_to_mag_impl::~pdu_complex_to_mag_impl() {}

// Handles Incoming Messages
void pdu_complex_to_mag_impl::handle_msg(pmt::pmt_t msg) 
{
    if (!pmt::is_pair(msg)) {
        GR_LOG_WARN(d_logger, "Received non-PDU message, ignoring.");
        return;
    }

    // Extract metadata and data vector
    pmt::pmt_t key = pmt::car(msg);
    pmt::pmt_t value = pmt::cdr(msg);

    if (!pmt::is_c32vector(value)) {
        GR_LOG_WARN(d_logger, "Received non-complex vector, ignoring.");
        return;
    }

    // Convert PMT vector to ArrayFire array
    size_t n = pmt::length(value);
    GR_LOG_INFO(d_logger, "Received PDU with length: " + std::to_string(n));

    std::vector<gr_complex> host_data(n);
    memcpy(host_data.data(), pmt::c32vector_elements(value, n), n * sizeof(gr_complex));

    af::array af_data(n, reinterpret_cast<af::cfloat*>(host_data.data()));
    GR_LOG_INFO(d_logger, "ArrayFire data shape: " + std::to_string(af_data.dims(0)));

    // Compute magnitude
    af::array magnitude = af::abs(af_data);

    // Convert to PMT (float instead of double)
    std::vector<float> host_magnitude(n);
    magnitude.host(host_magnitude.data());

    pmt::pmt_t mag_pmt = pmt::init_f32vector(n, host_magnitude.data());

    // Publish the message
    message_port_pub(out_port, pmt::cons(key, mag_pmt));
}

} /* namespace harmonia */
} /* namespace gr */
