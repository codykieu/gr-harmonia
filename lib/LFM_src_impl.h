/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_LFM_SRC_IMPL_H
#define INCLUDED_HARMONIA_LFM_SRC_IMPL_H

#include <gnuradio/harmonia/LFM_src.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>
#include <plasma_dsp/pulsed_waveform.h>

namespace gr
{
  namespace harmonia
  {

    class LFM_src_impl : public LFM_src
    {
    private:
      // Waveform parameters
      double bandwidth;
      double start_freq;
      double center_freq;
      double pulse_width;
      double pulse_width2;
      double samp_rate;
      double prf;
      int sdr_id;

      // Waveform object and IQ data
      pmt::pmt_t d_data;
      double alpha_hat, phi_hat, gamma_hat;
      af::array waveform;
      
      // Metadata fields
      pmt::pmt_t label_key;
      pmt::pmt_t sample_rate_key;
      pmt::pmt_t bandwidth_key;
      pmt::pmt_t duration_key;
      pmt::pmt_t prf_key;
      pmt::pmt_t meta;
      pmt::pmt_t cd_val;
      pmt::pmt_t cb_val;
      pmt::pmt_t cp_val;

      void handle_msg(pmt::pmt_t msg);

    public:
      LFM_src_impl(double bandwidth,
                   double start_freq,
                   double center_freq,
                   double pulse_width,
                   double pulse_width2,
                   double samp_rate,
                   double prf,
                   int sdr_id);
      ~LFM_src_impl();

      bool start() override;

      void init_meta_dict(const std::string &bandwidth_key,
                          const std::string &duration_key,
                          const std::string &sample_rate_key,
                          const std::string &label_key,
                          const std::string &prf_key);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_LFM_SRC_IMPL_H */
