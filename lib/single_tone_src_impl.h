/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_SINGLE_TONE_SRC_IMPL_H
#define INCLUDED_HARMONIA_SINGLE_TONE_SRC_IMPL_H

#include <gnuradio/harmonia/single_tone_src.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>
#include <plasma_dsp/pulsed_waveform.h>

namespace gr
{
  namespace harmonia
  {

    class single_tone_src_impl : public single_tone_src
    {
    private:
      // Message ports
      const pmt::pmt_t msg_port;
      const pmt::pmt_t out_port;
      pmt::pmt_t d_msg;

      // Waveform parameters
      double frequency;
      double center_freq;
      double phase;
      double pulse_width;
      double samp_rate;
      double prf;
      int sdr_id;

      // Waveform object and IQ data
      pmt::pmt_t d_data;
      double alpha_hat;
      
      // Metadata fields
      pmt::pmt_t label_key;
      pmt::pmt_t sample_rate_key;
      pmt::pmt_t frequency_key;
      pmt::pmt_t phase_key;
      pmt::pmt_t duration_key;
      pmt::pmt_t prf_key;
      pmt::pmt_t meta;

      void handle_msg(pmt::pmt_t msg);
      void buffer_corrector(std::vector<gr_complex> &vec);

    public:
      single_tone_src_impl(double frequency,
        double center_freq,
                           double phase,
                           double pulse_width,
                           double samp_rate,
                           double prf,
                           int sdr_id);
      ~single_tone_src_impl();

      bool start() override;

      void init_meta_dict(const std::string &frequency_key,
                          const std::string &phase_key,
                          const std::string &duration_key,
                          const std::string &sample_rate_key,
                          const std::string &label_key,
                          const std::string &prf_key);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_SINGLE_TONE_SRC_IMPL_H */
