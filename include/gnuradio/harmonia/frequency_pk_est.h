/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_FREQUENCY_PK_EST_H
#define INCLUDED_HARMONIA_FREQUENCY_PK_EST_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>
#include <gnuradio/harmonia/device.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API frequency_pk_est : virtual public gr::block
{
public:
    typedef std::shared_ptr<frequency_pk_est> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of harmonia::frequency_pk_est.
     *
     * To avoid accidental use of raw pointers, harmonia::frequency_pk_est's
     * constructor is in a private implementation
     * class. harmonia::frequency_pk_est::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t fft_ratio, double pulse_width, double cap_length,
        double samp_rate, double NLLS_iter, int sdr_id, bool enable_out);

    virtual void set_msg_queue_depth(size_t) = 0;

    virtual void set_backend(Device::Backend) = 0;
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_FREQUENCY_PK_EST_H */
