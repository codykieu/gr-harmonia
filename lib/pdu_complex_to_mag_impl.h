/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_PDU_COMPLEX_TO_MAG_IMPL_H
#define INCLUDED_HARMONIA_PDU_COMPLEX_TO_MAG_IMPL_H

#include <gnuradio/harmonia/pdu_complex_to_mag.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>

namespace gr {
namespace harmonia {

class pdu_complex_to_mag_impl : public pdu_complex_to_mag
{
private:
    pmt::pmt_t in_port;
    pmt::pmt_t out_port;

public:
    pdu_complex_to_mag_impl(); 
    ~pdu_complex_to_mag_impl();
    
    void handle_msg(pmt::pmt_t msg);
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_PDU_COMPLEX_TO_MAG_IMPL_H */
