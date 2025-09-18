/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_TIME_PK_EST_IMPL_H
#define INCLUDED_HARMONIA_TIME_PK_EST_IMPL_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/harmonia/time_pk_est.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <plasma_dsp/fft.h>
#include <arrayfire.h>
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <complex>
#include <stdexcept>

namespace gr
{
  namespace harmonia
  {

    class time_pk_est_impl : public time_pk_est
    {
    private:
      // Parameters
      double samp_rate;
      double bandwidth;
      double wait_time;
      double sample_delay;
      double NLLS_iter;
      int sdr_id;
      double wire_delay;
      double wire_delay_tx;
      double t_delay;
      bool enable_out;

      // Variables
      af::array d_match_filt;
      af::Backend d_backend;
      size_t d_msg_queue_depth;
      double t_est;
      double p_est;
      std::vector<double> d_sdr1_time_est;
      std::vector<double> d_sdr2_time_est;
      std::vector<double> d_sdr3_time_est;
      std::vector<double> d_sdr1_phase_est;
      std::vector<double> d_sdr2_phase_est;
      std::vector<double> d_sdr3_phase_est;
      int d_rx_count;
      double alpha1, alpha2, alpha3, alpha_hat;


      // Message Ports
      pmt::pmt_t d_tx_port;
      pmt::pmt_t d_rx_port;
      pmt::pmt_t d_out_port;
      pmt::pmt_t d_tp_out_port;

      // Metadata
      pmt::pmt_t d_meta;
      pmt::pmt_t d_data;
      pmt::pmt_t d_tp_meta;
      pmt::pmt_t sdr_pmt;

      af::array sinc(const af::array &x);

      void handle_tx_msg(pmt::pmt_t);
      void handle_rx_msg(pmt::pmt_t);
      void handle_clock_drift(pmt::pmt_t msg);

    public:
      time_pk_est_impl(double samp_rate, double bandwidth, double wait_time, double sample_delay, double NLLS_iter, int sdr_id, bool enable_out);
      ~time_pk_est_impl();

      void set_msg_queue_depth(size_t) override;
      void set_backend(Device::Backend) override;
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_TIME_PK_EST_IMPL_H */
