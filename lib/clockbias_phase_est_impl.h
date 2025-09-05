/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_CLOCKBIAS_PHASE_EST_IMPL_H
#define INCLUDED_HARMONIA_CLOCKBIAS_PHASE_EST_IMPL_H

#include <gnuradio/harmonia/clockbias_phase_est.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>
#include <chrono>
#include <iomanip>

namespace gr
{
  namespace harmonia
  {

    class clockbias_phase_est_impl : public clockbias_phase_est
    {
    private:
      // Parameters
      int num_platforms;
      double center_freq;
      double samp_rate;
      double pulse_width;
      double SNR;
      bool bias_status, phase_status;

      // Variables
      std::vector<std::vector<double>> time_matrix;
      std::vector<std::vector<double>> phase_matrix;
      std::vector<bool> check_time;
      std::vector<bool> check_phase;
      double alpha1, alpha2, alpha3;
      af::array TOF_est, Phase_est;
      af::array cb_est, x_gamma_est, R_est_lower;
      
      // Object and data
      pmt::pmt_t d_data;

      // Metadata fields
      pmt::pmt_t meta;
      pmt::pmt_t key;
      pmt::pmt_t d_meta = pmt::make_dict();
      pmt::pmt_t cd_meta = pmt::make_dict();

      // Functions
      af::array to_af_array(const std::vector<std::vector<double>> &matrix);
      void handle_msg(pmt::pmt_t msg);
      void handle_clock_drift(pmt::pmt_t msg);

    public:
      clockbias_phase_est_impl(int num_platforms,
                               double center_freq,
                               double samp_rate,
                               double pulse_width,
                               double SNR,
                               bool bias_status,
                               bool phase_status);
      ~clockbias_phase_est_impl();
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_CLOCKBIAS_PHASE_EST_IMPL_H */
