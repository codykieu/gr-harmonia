/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_COMPENSATION_IMPL_H
#define INCLUDED_HARMONIA_COMPENSATION_IMPL_H

#include <gnuradio/harmonia/compensation.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>
#include <plasma_dsp/pulsed_waveform.h>
#include <plasma_dsp/fft.h>

namespace gr
{
  namespace harmonia
  {

    class compensation_impl : public compensation
    {
    private:
      // Parameters
      double center_freq;
      double samp_rate;
      int sdr_id;

      // Variables
      std::array<double, 3> alpha = {{1.0, 1.0, 1.0}};
      std::array<double, 3> phi = {{0.0, 0.0, 0.0}};
      std::array<double, 3> gamma = {{0.0, 0.0, 0.0}};

      bool alpha_ready = false;
      bool phi_ready = false;
      bool gamma_ready = false;
      double alpha_hat, phi_hat, gamma_hat;
      double t_delay;

      // Metadata
      pmt::pmt_t d_meta;
      pmt::pmt_t d_data;

      void handle_msg(pmt::pmt_t);
      void handle_cd_msg(pmt::pmt_t);
      void handle_cb_msg(pmt::pmt_t);
      void handle_cp_msg(pmt::pmt_t);

    public:
      compensation_impl(double center_freq, double samp_rate, int sdr_id);
      ~compensation_impl();
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_COMPENSATION_IMPL_H */
