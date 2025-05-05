/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_SDR_TAGGER_IMPL_H
#define INCLUDED_HARMONIA_SDR_TAGGER_IMPL_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/harmonia/SDR_tagger.h>
#include <gnuradio/harmonia/pmt_constants.h>

namespace gr
{
  namespace harmonia
  {

    class SDR_tagger_impl : public SDR_tagger
    {
    private:
      pmt::pmt_t d_sdr_num_key;

      // Message Ports
      pmt::pmt_t d_out_port;
      pmt::pmt_t d_in_port;

      // Waveform object and IQ data
      pmt::pmt_t d_data;

      // Metadata
      pmt::pmt_t d_meta;

      void handle_msg(pmt::pmt_t msg);

    public:
      SDR_tagger_impl(const std::string& sdr_num_key);
      ~SDR_tagger_impl();
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_SDR_TAGGER_IMPL_H */
