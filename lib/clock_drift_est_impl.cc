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

    clock_drift_est::sptr clock_drift_est::make(int num_platforms, int platform_num, double baseband_freq, double center_freq, double samp_rate, double pulse_width, double SNR)
    {
      return gnuradio::make_block_sptr<clock_drift_est_impl>(num_platforms, platform_num, baseband_freq, center_freq, samp_rate, pulse_width, SNR);
    }

    /*
     * The private constructor
     */
    clock_drift_est_impl::clock_drift_est_impl(int num_platforms, int platform_num, double baseband_freq, double center_freq, double samp_rate, double pulse_width, double SNR)
        : gr::block("clock_drift_est",
                    gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          msg_port(PMT_HARMONIA_IN),
          out_port(PMT_HARMONIA_OUT),
          num_platforms(num_platforms),
          platform_num(platform_num),
          baseband_freq(baseband_freq),
          center_freq(center_freq),
          samp_rate(samp_rate),
          pulse_width(pulse_width),
          SNR(SNR)
    {
      meta = pmt::make_dict();
      message_port_register_in(msg_port);
      message_port_register_out(out_port);

      set_msg_handler(msg_port, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    clock_drift_est_impl::~clock_drift_est_impl() {}

    void clock_drift_est_impl::handle_msg(pmt::pmt_t msg)
    {
      // Ensure message is a dictionary
      if (!pmt::is_dict(msg))
      {
        GR_LOG_ERROR(d_logger, "Message is not a dictionary!");
        return;
      }

      // Map SDR PMT symbols to indices
      std::map<pmt::pmt_t, int> sdr_idx = {
          {PMT_HARMONIA_SDR1, 0},
          {PMT_HARMONIA_SDR2, 1},
          {PMT_HARMONIA_SDR3, 2}};

      // Transmitting SDR, Order of SDR it's receiving
      std::map<int, std::vector<int>> tx_to_rx_idx = {
          {0, {1, 2}}, // SDR1 TX → SDR2, SDR3
          {1, {0, 2}}, // SDR2 TX → SDR1, SDR3
          {2, {0, 1}}, // SDR3 TX → SDR1, SDR2
      };

      std::vector<std::vector<double>> A_est;

      std::vector<pmt::pmt_t> tx_order = {
          PMT_HARMONIA_SDR1,
          PMT_HARMONIA_SDR2,
          PMT_HARMONIA_SDR3,
      };

      for (const auto &tx_key : tx_order)
      {
        if (!pmt::dict_has_key(msg, tx_key))
          continue;

        pmt::pmt_t vec_pmt = pmt::dict_ref(msg, tx_key, pmt::PMT_NIL);
        if (!pmt::is_f32vector(vec_pmt))
          continue;

        int i = sdr_idx[tx_key];
        const auto &rx_indices = tx_to_rx_idx[i];
        std::vector<float> freq_estimates_f32 = pmt::f32vector_elements(vec_pmt);
        // Convert vector into double
        std::vector<double> freq_estimates(freq_estimates_f32.begin(), freq_estimates_f32.end());

        if (freq_estimates.size() != rx_indices.size())
        {
          GR_LOG_ERROR(d_logger, "Mismatch in RX index size vs freq estimate vector");
          continue;
        }

        // Allocating frequency estimates into matrix
        for (size_t k = 0; k < freq_estimates.size(); ++k)
        {
          int j = rx_indices[k];
          double f_est = freq_estimates[k];

          std::vector<double> row(num_platforms, 0.0);
          row[j] = f_est + center_freq;
          row[i] = -(baseband_freq + center_freq);
          A_est.push_back(row);
        }
      }

      // Assumption of Clock Drift of SDR1 = 1.0
      std::vector<double> constraint(num_platforms, 0.0);
      constraint[0] = center_freq;
      A_est.push_back(constraint);

      // Flatten in column-major order for ArrayFire
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
      af::array y = af::join(0, af::constant(0.0, A_est.size() - 1, f64), af::constant(1.0, 1, f64));

      // Compute variance
      double var_f = 3 / (2 * std::pow(af::Pi, 2.0) * std::pow(pulse_width, 3.0) * samp_rate *
                          std::pow(10.0, SNR / 10.0) * (1.0 - std::pow(1.0 / (pulse_width * samp_rate), 2.0)));

      // Diagonalize noise/weight matrix according to SNR
      af::array diag_vals = af::join(0,
                                     af::constant(var_f, A_est.size() - 1, f64),
                                     af::constant(1e-12, 1, f64));
      af::array Cov_mat = af::diag(diag_vals, 0, false);
      af::array inv_Cov_mat = af::inverse(Cov_mat);

      // Solve Weighted Least Sqares
      af::array At_Cinv = af::matmulTN(A_mat, inv_Cov_mat);
      af::array lhs = af::matmul(At_Cinv, A_mat);
      af::array rhs = af::matmul(At_Cinv, y);
      af::array x_alpha = af::solve(lhs, rhs) * center_freq;

      // Output estimates
      std::vector<double> x_host(x_alpha.elements());
      x_alpha.host(x_host.data());

      // Print estimates
      std::cout << "x_alpha (Hz):" << std::endl;
      for (double val : x_host)
        std::cout << std::fixed << std::setprecision(13) << val << std::endl;

      // Add to metadata
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, pmt::from_double(x_host[0]));
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, pmt::from_double(x_host[1]));
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, pmt::from_double(x_host[2]));
      meta = pmt::dict_add(meta, pmt::intern("clock_drift_enable"), pmt::PMT_T);
      // Export message
      // message_port_pub(out_port, meta);
      pmt::pmt_t empty_vec = pmt::make_u8vector(0, 0);
      message_port_pub(out_port, pmt::cons(meta, empty_vec));
    }

  } /* namespace harmonia */
} /* namespace gr */
