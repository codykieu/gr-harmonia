/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_CLOCK_DRIFT_EST_IMPL_H
#define INCLUDED_HARMONIA_CLOCK_DRIFT_EST_IMPL_H

#include <gnuradio/harmonia/clock_drift_est.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>
#include <chrono>
#include <iomanip> 
namespace gr
{
  namespace harmonia
  {

    class clock_drift_est_impl : public clock_drift_est
    {
    private:
      const pmt::pmt_t msg_port;
      const pmt::pmt_t out_port;
      pmt::pmt_t d_msg;

      // Parameters
      int num_platforms;
      int platform_num;
      double baseband_freq;
      double center_freq;
      double samp_rate;
      double pulse_width;
      double SNR;
      double freq_val;
      af::array A_mat;
      
      // Object and data
      pmt::pmt_t d_data;

      // Metadata fields
      pmt::pmt_t meta;
      pmt::pmt_t key;
      pmt::pmt_t freq_val_pmt;

      void handle_msg(pmt::pmt_t msg);

    public:
      clock_drift_est_impl(int num_platforms,
                           int platform_num,
                           double baseband_freq,
                           double center_freq, 
                           double samp_rate, 
                           double pulse_width, 
                           double SNR);
      ~clock_drift_est_impl();
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_CLOCK_DRIFT_EST_IMPL_H */
