/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clockbias_phase_est_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    clockbias_phase_est::sptr clockbias_phase_est::make(int num_platforms,
                                                        int platform_num,
                                                        double baseband_freq,
                                                        double center_freq,
                                                        double samp_rate,
                                                        double pulse_width,
                                                        double SNR)
    {
      return gnuradio::make_block_sptr<clockbias_phase_est_impl>(num_platforms,
                                                                 platform_num,
                                                                 baseband_freq,
                                                                 center_freq,
                                                                 samp_rate,
                                                                 pulse_width,
                                                                 SNR);
    }

    /*
     * The private constructor
     */
    clockbias_phase_est_impl::clockbias_phase_est_impl(int num_platforms,
                                                       int platform_num,
                                                       double baseband_freq,
                                                       double center_freq,
                                                       double samp_rate,
                                                       double pulse_width,
                                                       double SNR)
        : gr::block("clockbias_phase_est",
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
    clockbias_phase_est_impl::~clockbias_phase_est_impl() {}

    af::array clockbias_phase_est_impl::to_af_array(const std::vector<std::vector<float>> &matrix)
    {
      int rows = matrix.size();
      int cols = matrix[0].size();
      std::vector<float> flat(rows * cols);

      for (int r = 0; r < rows; ++r)
      {
        for (int c = 0; c < cols; ++c)
        {
          flat[c * rows + r] = matrix[r][c]; // column-major flattening
        }
      }

      return af::array(rows, cols, flat.data()); // no dtype here!
    }

    af::array wrapToPi_af(const af::array &angle)
    {
      const double pi = M_PI;
      af::array wrapped = af::mod(angle + pi, 2 * pi);
      wrapped = af::select(wrapped < 0, wrapped + 2 * pi, wrapped);
      return wrapped - pi;
    }

    void clockbias_phase_est_impl::handle_msg(pmt::pmt_t msg)
    {
      // Ensure message is a dictionary
      if (!pmt::is_dict(msg))
      {
        GR_LOG_ERROR(d_logger, "Message is not a dictionary!");
        return;
      }

      // Map SDR PMT symbols to matrix indices
      std::map<pmt::pmt_t, int> sdr_idx = {
          {PMT_HARMONIA_SDR1, 0},
          {PMT_HARMONIA_SDR2, 1},
          {PMT_HARMONIA_SDR3, 2}};

      std::vector<pmt::pmt_t> tx_order = {
          PMT_HARMONIA_SDR1,
          PMT_HARMONIA_SDR2,
          PMT_HARMONIA_SDR3};

      // === Build TOF Matrix ===
      std::vector<std::vector<float>> tof_matrix(num_platforms, std::vector<float>(num_platforms, 0.0f));

      for (const auto &tx_key : tx_order)
      {
        if (!pmt::dict_has_key(msg, tx_key))
          continue;

        pmt::pmt_t vec_pmt = pmt::dict_ref(msg, tx_key, pmt::PMT_NIL);
        if (!pmt::is_f32vector(vec_pmt))
          continue;

        int tx_idx = sdr_idx[tx_key];
        std::vector<float> estimates = pmt::f32vector_elements(vec_pmt);

        // Determine RX indices (≠ TX)
        std::vector<int> rx_indices;
        for (const auto &pair : sdr_idx)
        {
          int rx_idx = pair.second;
          if (rx_idx != tx_idx)
            rx_indices.push_back(rx_idx);
        }

        if (estimates.size() != rx_indices.size())
        {
          GR_LOG_ERROR(d_logger, "Mismatch in RX index size vs estimate vector");
          continue;
        }

        for (size_t k = 0; k < estimates.size(); ++k)
        {
          int rx_idx = rx_indices[k];
          tof_matrix[tx_idx][rx_idx] = estimates[k]; // TX → RX
        }
      }

      // === Build PHI Matrix ===
      // Map phase PMT keys to corresponding SDR index
      std::map<pmt::pmt_t, int> pmt_phase_keys = {
          {PMT_HARMONIA_P_SDR1, 0},
          {PMT_HARMONIA_P_SDR2, 1},
          {PMT_HARMONIA_P_SDR3, 2}};

      std::vector<std::vector<float>> phi_matrix(num_platforms, std::vector<float>(num_platforms, 0.0f));

      for (const auto &pair : pmt_phase_keys)
      {
        pmt::pmt_t p_key = pair.first;
        int tx_idx = pair.second;

        if (!pmt::dict_has_key(msg, p_key))
          continue;

        pmt::pmt_t vec_pmt = pmt::dict_ref(msg, p_key, pmt::PMT_NIL);
        if (!pmt::is_f32vector(vec_pmt))
          continue;

        std::vector<float> estimates = pmt::f32vector_elements(vec_pmt);

        // Determine RX indices (≠ TX)
        std::vector<int> rx_indices;
        for (const auto &subpair : sdr_idx)
        {
          int rx_idx = subpair.second;
          if (rx_idx != tx_idx)
            rx_indices.push_back(rx_idx);
        }

        if (estimates.size() != rx_indices.size())
        {
          GR_LOG_ERROR(d_logger, "Mismatch in RX index size vs phase vector");
          continue;
        }

        for (size_t k = 0; k < estimates.size(); ++k)
        {
          int rx_idx = rx_indices[k];
          phi_matrix[tx_idx][rx_idx] = estimates[k];
        }
      }

      // === Convert and symmetrize TOF & PHI ===
      auto convert_and_symmetrize = [this](const std::vector<std::vector<float>> &m) -> af::array
      {
        std::vector<float> flat(num_platforms * num_platforms);
        for (int r = 0; r < num_platforms; ++r)
          for (int c = 0; c < num_platforms; ++c)
            flat[c * num_platforms + r] = m[r][c]; // column-major
        af::array A(num_platforms, num_platforms, flat.data());
        return (A - A.T()) / 2.0; // antisymmetric form
      };

      TOF_est = convert_and_symmetrize(tof_matrix);
      Phase_est = to_af_array(phi_matrix);

      // Print for debug
      af_print(TOF_est);
      af_print(Phase_est);
      af::array gamma = Phase_est + 2 * M_PI * center_freq * TOF_est;
      gamma = wrapToPi_af(gamma);
      af_print(gamma);

      // Step 1: Flatten gamma to 1D
      af::array gamma_flat = af::flat(gamma);

      // Step 2: Get non-zero elements
      af::array nonzero_idx = af::where(gamma_flat != 0);
      af::array gamma_nonzero = gamma_flat(nonzero_idx);

      // Step 3: Prepare a nonzero gamma array
      af::array y_err = af::constant(0.0, 2*num_platforms+1, 1, gamma_nonzero.type());

      // Step 4: Overwrite beginning of gamma_7x1 with available nonzero values
      int n_nonzero = std::min(2*num_platforms+1, static_cast<int>(gamma_nonzero.elements()));
      if (n_nonzero > 0)
      {
        y_err(af::seq(n_nonzero)) = gamma_nonzero(af::seq(n_nonzero));
      }

      af_print(y_err);

      // meta = pmt::dict_add(meta, pmt::intern("carrier_phase"), pmt::from_float(clock_drift_est1));

      // // y = Ax
      // af::array y = af::join(0, af::constant(0, 2 * num_platforms), af::constant(1, 1));

      // Variance of frequency
      double var_f = 3 / (2 * std::pow(af::Pi, 2) * std::pow(pulse_width, 3) * samp_rate * std::pow(10.0, (SNR / 10.0)) * (1 - std::pow((1 / (pulse_width * samp_rate)), 2)));
      // std::cout << "var_f: " << var_f << std::endl;

      // Create the diagonal values
      af::array diag_vals = af::flat(af::join(0, (af::constant(var_f, 2 * num_platforms)), af::constant(1e-9f, 1)));

      // Create the diagonal matrix
      af::array Cov_mat = af::diag(diag_vals, 0, false);

      // Compute inverse of Covariance matrix
      af::array inv_Cov_mat = af::inverse(Cov_mat);


      // af::array x_gamma = af::matmul(af::matmul(af::matmul(af::inverse(af::matmul(af::matmul(A_gamma.T(), inv_Cov_mat), A_gamma)), A_gamma.T()), inv_Cov_mat), (y_err + gamma_wrap));
      // af_print(x_gamma);
      // std::vector<float> x_host(x_gamma.elements());
      // x_gamma.host(x_host.data());

      // std::cout << "x_alpha (Hz):" << std::endl;
      // for (size_t i = 0; i < x_host.size(); ++i)
      // {
      //   std::cout << std::fixed << std::setprecision(10) << x_host[i] << std::endl;
      // }
      // float clock_drift_est1 = x_alpha(0).scalar<float>();
      // float clock_drift_est2 = x_alpha(1).scalar<float>();
      // float clock_drift_est3 = x_alpha(2).scalar<float>();

      // meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, pmt::from_float(clock_drift_est1));
      // meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, pmt::from_float(clock_drift_est2));
      // meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, pmt::from_float(clock_drift_est3));

      message_port_pub(out_port, meta);
    }

  } /* namespace harmonia */
} /* namespace gr */
