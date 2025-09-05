/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clock_drift_est_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    clock_drift_est::sptr clock_drift_est::make(int num_platforms, double baseband_freq, double center_freq, double samp_rate, double pulse_width, double SNR)
    {
      return gnuradio::make_block_sptr<clock_drift_est_impl>(num_platforms, baseband_freq, center_freq, samp_rate, pulse_width, SNR);
    }

    /*
     * The private constructor
     */
    clock_drift_est_impl::clock_drift_est_impl(int num_platforms, double baseband_freq, double center_freq, double samp_rate, double pulse_width, double SNR)
        : gr::block("clock_drift_est",
                    gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          num_platforms(num_platforms),
          baseband_freq(baseband_freq),
          center_freq(center_freq),
          samp_rate(samp_rate),
          pulse_width(pulse_width),
          SNR(SNR),
          d_store(num_platforms*(num_platforms-1), 0.0),
          d_got(num_platforms*(num_platforms-1), false)
    {
      meta = pmt::make_dict();
      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_in(PMT_HARMONIA_IN2);
      message_port_register_in(PMT_HARMONIA_IN3);
      message_port_register_out(PMT_HARMONIA_OUT);

      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_IN2, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_IN3, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    clock_drift_est_impl::~clock_drift_est_impl() {}

    void clock_drift_est_impl::handle_msg(pmt::pmt_t msg)
    {
      // 1) Extract incoming dict
      pmt::pmt_t dict;
      if (pmt::is_pair(msg))
        dict = pmt::car(msg);
      else if (pmt::is_dict(msg))
        dict = msg;
      else
        return;

      // Extract SDR
      pmt::pmt_t rx_pmt = pmt::dict_ref(dict, pmt::intern("rx_id"), pmt::PMT_NIL);
      int rx = -1;
      if (rx_pmt == PMT_HARMONIA_SDR1)
        rx = 0;
      else if (rx_pmt == PMT_HARMONIA_SDR2)
        rx = 1;
      else if (rx_pmt == PMT_HARMONIA_SDR3)
        rx = 2;
      else
        return;

      // Map (rx,tx)
      // Order: (2,1)->0, (3,1)->1, (1,2)->2, (3,2)->3, (1,3)->4, (2,3)->5
      static const int idx_map[3][3] = {
          /*tx=1*/ {-1, 2, 4}, // rx=1,2,3
          /*tx=2*/ {0, -1, 5},
          /*tx=3*/ {1, 3, -1}};

      // For each other SDR where tx != rx, extract its float-vector
      static const pmt::pmt_t sdr_syms[3] = {
          PMT_HARMONIA_SDR1,
          PMT_HARMONIA_SDR2,
          PMT_HARMONIA_SDR3};

      for (int tx = 0; tx < 3; ++tx)
      {
        if (tx == rx)
          continue;
        pmt::pmt_t vec_pmt = pmt::dict_ref(dict, sdr_syms[tx], pmt::PMT_NIL);
        if (!pmt::is_f32vector(vec_pmt))
          return; // wait until this SDR’s data arrives
        auto tmpf = pmt::f32vector_elements(vec_pmt);
        if (tmpf.size() != 1)
          return;               
        double f_est = tmpf[0]; 

        int idx = idx_map[rx][tx];
        d_store[idx] = f_est + center_freq; 
        d_got[idx] = true;                  
      }

      // Wait until all six values have been received
      if (!std::all_of(d_got.begin(), d_got.end(), [](bool b)
                       { return b; }))
        return;

      // Build A_est
      std::vector<std::vector<double>> A_est;
      A_est.reserve(d_store.size() + 1);
      for (int n = 0; n < (int)d_store.size(); ++n)
      {
        int rx_i, tx_i;
        switch (n)
        {
        case 0:
          rx_i = 1;
          tx_i = 0;
          break; // (2,1)
        case 1:
          rx_i = 2;
          tx_i = 0;
          break; // (3,1)
        case 2:
          rx_i = 0;
          tx_i = 1;
          break; // (1,2)
        case 3:
          rx_i = 2;
          tx_i = 1;
          break; // (3,2)
        case 4:
          rx_i = 0;
          tx_i = 2;
          break; // (1,3)
        case 5:
          rx_i = 1;
          tx_i = 2;
          break; // (2,3)
        }
        std::vector<double> row(num_platforms, 0.0);
        row[rx_i] = d_store[n];
        row[tx_i] = -(baseband_freq + center_freq);
        A_est.push_back(row);
      }
      // Assumption alpha_1 = 1.0
      {
        std::vector<double> ref(num_platforms, 0.0);
        ref[0] = center_freq;
        A_est.push_back(ref);
      }

      // Flatten for ArrayFire
      std::vector<double> flat(num_platforms * A_est.size(), 0.0);
      for (size_t r = 0; r < A_est.size(); ++r)
      {
        for (int c = 0; c < num_platforms; ++c)
        {
          flat[c * A_est.size() + r] = A_est[r][c];
        }
      }

      // Matrix for Linear System of Equations
      af::dim4 dims(A_est.size(), num_platforms);
      A_mat = af::array(dims, flat.data(), afHost);
      A_mat = A_mat.as(f64);
      // af_print(A_mat);

      af::array y = af::join(0, af::constant(0.0, A_est.size() - 1, f64), af::constant(center_freq, 1, f64));

      // Compute variance
      double var_f = 3 / (2 * std::pow(af::Pi, 2.0) * std::pow(pulse_width, 3.0) * samp_rate *
                          std::pow(10.0, SNR / 10.0) * (1.0 - std::pow(1.0 / (pulse_width * samp_rate), 2.0)));

      // Diagonalize noise/weight matrix according to SNR
      af::array diag_vals = af::join(0,
                                     af::constant(var_f, A_est.size() - 1, f64),
                                     af::constant(1e-12, 1, f64));
      af::array Cov_mat = af::diag(diag_vals, 0, false);
      af::array inv_Cov_mat = af::inverse(Cov_mat);

      // Solve Weighted Least Squares
      af::array At_Cinv = af::matmulTN(A_mat, inv_Cov_mat);
      af::array lhs = af::matmul(At_Cinv, A_mat);
      af::array rhs = af::matmul(At_Cinv, y);
      af::array x_alpha = af::solve(lhs, rhs);

      // Output estimates
      std::vector<double> x_host(x_alpha.elements());
      x_alpha.host(x_host.data());

      // Print estimates
      std::cout << "x_alpha (Hz):" << std::endl;
      for (double val : x_host)
        std::cout << std::fixed << std::setprecision(13) << val << std::endl;

      // Publish results once
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, pmt::from_double(1.0));
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, pmt::from_double(1.0));
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, pmt::from_double(1.0000));
      // meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, pmt::from_double(x_host[0]));
      // meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, pmt::from_double(x_host[1]));
      // meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, pmt::from_double(x_host[2]));
      meta = pmt::dict_add(meta, pmt::intern("clock_drift_enable"), pmt::PMT_T);
      message_port_pub(PMT_HARMONIA_OUT, meta);

      // pmt::pmt_t empty_vec = pmt::make_u8vector(0, 0);
      // message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, empty_vec));

      // Reset for next frame
      meta = pmt::make_dict();
    }

  } /* namespace harmonia */
} /* namespace gr */
