/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_PDU_FFT_H
#define INCLUDED_HARMONIA_PDU_FFT_H

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
class HARMONIA_API pdu_fft : virtual public gr::block
{
public:
    typedef std::shared_ptr<pdu_fft> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of harmonia::pdu_fft.
     *
     * To avoid accidental use of raw pointers, harmonia::pdu_fft's
     * constructor is in a private implementation
     * class. harmonia::pdu_fft::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t nfft);

    virtual void set_msg_queue_depth(size_t) = 0;

    virtual void set_backend(Device::Backend) = 0;

    virtual void set_metadata_keys(const std::string& fft_size_key) = 0;
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_PDU_FFT_H */
