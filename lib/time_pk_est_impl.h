/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_TIME_PK_EST_IMPL_H
#define INCLUDED_HARMONIA_TIME_PK_EST_IMPL_H

#include <gnuradio/harmonia/time_pk_est.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>

namespace gr
{
  namespace harmonia
  {

    class time_pk_est_impl : public time_pk_est
    {
    private:
      // Parameters
      double d_pulse_width;
      double d_cap_length;
      double d_samp_rate;
      double d_NLLS_iter;
      double d_NLLS_pts;
      double rx_time;

      // Variables
      size_t d_fftsize;
      size_t d_msg_queue_depth;
      double t_est;
      af::array d_match_filt;
      af::Backend d_backend;

      // Message Ports
      pmt::pmt_t d_tx_port;
      pmt::pmt_t d_rx_port;
      pmt::pmt_t d_out_port;

      // Metadata
      pmt::pmt_t d_meta;
      pmt::pmt_t d_data;
      pmt::pmt_t d_time_est_key;

      void handle_tx_msg(pmt::pmt_t);
      void handle_rx_msg(pmt::pmt_t);

    public:
      time_pk_est_impl(size_t nfft, double pulse_width, double cap_length,
                       double samp_rate, double NLLS_iter, double NLLS_pts);
      ~time_pk_est_impl();

      void set_msg_queue_depth(size_t) override;
      void set_backend(Device::Backend) override;
      void set_metadata_keys() override;
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_TIME_PK_EST_IMPL_H */
