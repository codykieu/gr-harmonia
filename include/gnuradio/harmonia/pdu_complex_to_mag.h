/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_PDU_COMPLEX_TO_MAG_H
#define INCLUDED_HARMONIA_PDU_COMPLEX_TO_MAG_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief converts Complex PDUs to Float Magnitude PDUs
 * \ingroup harmonia
 *
 * Converts a c32vector PDU to a f32vector PDU using magnitude
 */
class HARMONIA_API pdu_complex_to_mag : virtual public gr::block
{
public:
    typedef std::shared_ptr<pdu_complex_to_mag> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of harmonia::pdu_complex_to_mag.
     *
     * To avoid accidental use of raw pointers, harmonia::pdu_complex_to_mag's
     * constructor is in a private implementation
     * class. harmonia::pdu_complex_to_mag::make is the public interface for
     * creating new instances.
     */
    static sptr make();
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_PDU_COMPLEX_TO_MAG_H */
