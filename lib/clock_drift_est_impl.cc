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
      auto now = std::chrono::high_resolution_clock::now();
      double now_sec = std::chrono::duration<double>(now.time_since_epoch()).count();
      std::cout << std::fixed << std::setprecision(15); // 15 decimal places    

      // Run following code once
      bool first_run = true;

      if (first_run)
      {
        A_mat = af::constant(0, (2 * num_platforms + 1), (2 * num_platforms));
        A_mat(2 * num_platforms, 0) = 1.0; // Assumes SDR1 has no clock drift
        first_run = false;                 // Mark as done
      }

      // Handle dictionary inputs
      pmt::pmt_t key = pmt::car(msg);
      pmt::pmt_t value = pmt::cdr(msg);

      if (pmt::equal(key, PMT_HARMONIA_SDR1)) // SDR 1 TX
      {
        if (platform_num == 2)
        {
          // Convert the PMT vector to a std::vector<float>
          std::vector<float> value_vector = {-(static_cast<float>(center_freq) + static_cast<float>(baseband_freq)),
                                             (pmt::to_float(value) + static_cast<float>(center_freq)),
                                             0};
          af::array A_vec(value_vector.size(), value_vector.data());
          A_mat(0, af::seq(0, num_platforms - 1)) = A_vec;
        }
        if (platform_num == 3)
        {
          // Convert the PMT vector to a std::vector<float>
          std::vector<float> value_vector = {-(static_cast<float>(center_freq) + static_cast<float>(baseband_freq)),
                                             0,
                                             (pmt::to_float(value) + static_cast<float>(center_freq))};
          af::array A_vec(value_vector.size(), value_vector.data());
          A_mat(1, af::seq(0, num_platforms - 1)) = A_vec;
        }
      }

      else if (pmt::equal(key, PMT_HARMONIA_SDR2)) // SDR 2 TX
      {
        if (platform_num == 1)
        {
          // Convert the PMT vector to a std::vector<float>
          std::vector<float> value_vector = {(pmt::to_float(value) + static_cast<float>(center_freq)),
                                             -(static_cast<float>(center_freq) + static_cast<float>(baseband_freq)),
                                             0};
          af::array A_vec(value_vector.size(), value_vector.data());
          A_mat(2, af::seq(0, num_platforms - 1)) = A_vec;
        }
        if (platform_num == 3)
        {
          // Convert the PMT vector to a std::vector<float>
          std::vector<float> value_vector = {0,
                                             -(static_cast<float>(center_freq) + static_cast<float>(baseband_freq)),
                                             (pmt::to_float(value) + static_cast<float>(center_freq))};
          af::array A_vec(value_vector.size(), value_vector.data());
          A_mat(3, af::seq(0, num_platforms - 1)) = A_vec;
        }
      }

      else if (pmt::equal(key, PMT_HARMONIA_SDR3)) // SDR 3 TX
      {
        if (platform_num == 1)
        {
          // Convert the PMT vector to a std::vector<float>
          std::vector<float> value_vector = {(pmt::to_float(value) + static_cast<float>(center_freq)),
                                             0,
                                             -(static_cast<float>(center_freq) + static_cast<float>(baseband_freq))};
          af::array A_vec(value_vector.size(), value_vector.data());
          A_mat(4, af::seq(0, num_platforms - 1)) = A_vec;
        }
        if (platform_num == 2)
        {
          // Convert the PMT vector to a std::vector<float>
          std::vector<float> value_vector = {0,
                                             (pmt::to_float(value) + static_cast<float>(center_freq)),
                                             -(static_cast<float>(center_freq) + static_cast<float>(baseband_freq))};
          af::array A_vec(value_vector.size(), value_vector.data());
          A_mat(5, af::seq(0, num_platforms - 1)) = A_vec;
        }
      }
      else
      {
        GR_LOG_INFO(d_logger, "No SDR specified/found");
      }
      af_print(A_mat);
      // y = Ax
      af::array y = af::join(0, af::constant(0, 2 * num_platforms), af::constant(1, 1));
      af_print(y);

      // Variance of frequency
      double var_f = 3 / (2 * std::pow(af::Pi, 2) * std::pow(pulse_width, 3) * samp_rate * std::pow(10.0, (SNR / 10.0)) * (1 - std::pow((1 / (pulse_width * samp_rate)), 2)));
      // std::cout << "var_f: " << var_f << std::endl;

      // Create the diagonal values
      af::array diag_vals = af::flat(af::join(0, (af::constant(var_f, 2 * num_platforms)), af::constant(1e-9f, 1)));

      // Create the diagonal matrix
      af::array Cov_mat = af::diag(diag_vals, 0, false);

      // Compute inverse of Covariance matrix
      af::array inv_Cov_mat = af::inverse(Cov_mat);

      int N = 6;
      af::array A = af::constant(0.0f, N + 1, N); // 7x6 matrix

      // Fill in the custom pattern (assuming pattern based on visual)
      A(0, 0) = -1.001e9;
      A(0, 1) = 1.0192e9;
      A(0, 3) = -1.000e9;
      A(1, 0) = -1.001e9;
      A(1, 2) = 1.0187e9;
      A(1, 4) = -1.000e9;
      A(2, 0) = 1.0136e9;
      A(2, 1) = -1.001e9;
      A(2, 3) = -1.000e9;
      A(3, 1) = -1.001e9;
      A(3, 2) = 9.937e8;
      A(3, 5) = -1.000e9;
      A(4, 0) = 9.816e8;
      A(4, 2) = -1.001e9;
      A(4, 4) = -1.000e9;
      A(5, 1) = 1.0163e9;
      A(5, 2) = -1.001e9;
      A(5, 5) = -1.000e9;
      A(6, 0) = 1.000e9;
      af::array x_test = af::matmul(af::matmul(af::matmul(af::inverse(af::matmul(af::matmul(A.T(), inv_Cov_mat), A)), A.T()), inv_Cov_mat), y) * center_freq;
      af_print(x_test);
      af::array x_alpha = af::matmul(af::matmul(af::matmul(af::inverse(af::matmul(af::matmul(A_mat.T(), inv_Cov_mat), A_mat)), A_mat.T()), inv_Cov_mat), y) * center_freq;
      af_print(x_alpha);

      float clock_drift_est1 = x_test(0).scalar<float>();
      float clock_drift_est2 = x_test(1).scalar<float>();
      float clock_drift_est3 = x_test(2).scalar<float>();

      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, pmt::from_float(clock_drift_est1));
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, pmt::from_float(clock_drift_est2));
      meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, pmt::from_float(clock_drift_est3));

      message_port_pub(out_port, meta);
      auto now2 = std::chrono::high_resolution_clock::now();
      double now_sec2 = std::chrono::duration<double>(now2.time_since_epoch()).count();
      std::cout << "[INFO] Process Time: " << (now_sec2- now_sec) << " seconds" << std::endl;
  
    }

  } /* namespace harmonia */
} /* namespace gr */
