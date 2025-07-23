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
                                                        double center_freq,
                                                        double samp_rate,
                                                        double pulse_width,
                                                        double SNR,
                                                        bool bias_status,
                                                        bool phase_status)
    {
      return gnuradio::make_block_sptr<clockbias_phase_est_impl>(num_platforms,
                                                                 center_freq,
                                                                 samp_rate,
                                                                 pulse_width,
                                                                 SNR,
                                                                 bias_status,
                                                                 phase_status);
    }

    /*
     * The private constructor
     */
    clockbias_phase_est_impl::clockbias_phase_est_impl(int num_platforms,
                                                       double center_freq,
                                                       double samp_rate,
                                                       double pulse_width,
                                                       double SNR,
                                                       bool bias_status,
                                                       bool phase_status)
        : gr::block("clockbias_phase_est",
                    gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          num_platforms(num_platforms),
          center_freq(center_freq),
          samp_rate(samp_rate),
          pulse_width(pulse_width),
          SNR(SNR),
          bias_status(bias_status),
          phase_status(phase_status),
          time_matrix(num_platforms, std::vector<double>(num_platforms, 0.0)),
          phase_matrix(num_platforms, std::vector<double>(num_platforms, 0.0)),
          check_time(num_platforms, false),
          check_phase(num_platforms, false)
    {
      meta = pmt::make_dict();
      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_in(PMT_HARMONIA_IN2);
      message_port_register_in(PMT_HARMONIA_IN3);
      message_port_register_in(PMT_HARMONIA_CD_IN);
      message_port_register_out(PMT_HARMONIA_OUT);

      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_IN2, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_IN3, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CD_IN, [this](pmt::pmt_t msg)
                      { handle_clock_drift(msg); });
    }

    /*
     * Our virtual destructor.
     */
    clockbias_phase_est_impl::~clockbias_phase_est_impl() {}

    af::array clockbias_phase_est_impl::to_af_array(const std::vector<std::vector<double>> &matrix)
    {
      int rows = matrix.size();
      int cols = matrix[0].size();
      std::vector<double> flat(rows * cols);

      for (int r = 0; r < rows; ++r)
      {
        for (int c = 0; c < cols; ++c)
        {
          flat[c * rows + r] = matrix[r][c];
        }
      }

      return af::array(rows, cols, flat.data());
    }

    // Function to wrap Pi values between -pi to pi
    af::array wrapToPi_af(const af::array &angle)
    {
      const double pi = M_PI;
      const double twoπ = 2.0 * pi;
      af::array tmp = (angle + pi) / twoπ;
      af::array round_val = af::floor(tmp);

      return angle - (twoπ * round_val);
    }

    void clockbias_phase_est_impl::handle_clock_drift(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      double default_alpha = 1.0;
      alpha1 = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_SDR1, pmt::from_double(default_alpha)));
      alpha2 = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_SDR2, pmt::from_double(default_alpha)));
      alpha3 = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_SDR3, pmt::from_double(default_alpha)));
      // alpha1 = 1.0;
      // alpha2 = 1.0;
      // alpha3 = 1.0;
      // std::cout << std::fixed << std::setprecision(12)
      // << "alpha1 = " << alpha1 << "\n"
      // << "alpha2 = " << alpha2 << "\n"
      // << "alpha3 = " << alpha3 << std::endl;
    }

    void clockbias_phase_est_impl::handle_msg(pmt::pmt_t msg)
    {
      // Check for incoming message
      if (pmt::is_pair(msg))
      {
        pmt::pmt_t incoming_meta = pmt::car(msg);
        if (!pmt::is_dict(incoming_meta))
        {
          GR_LOG_ERROR(d_logger, "PDU car() is not a dict");
          return;
        }
        d_meta = pmt::dict_update(d_meta, incoming_meta);
      }
      else if (pmt::is_dict(msg))
      {
        d_meta = pmt::dict_update(d_meta, msg);
      }
      else
      {
        GR_LOG_WARN(d_logger, "Received neither PDU nor dict");
        return;
      }

      // Extract SDR RX ID
      pmt::pmt_t rx_pmt = pmt::dict_ref(d_meta, pmt::intern("rx_id"), pmt::PMT_NIL);
      int rx = -1;
      if (rx_pmt == PMT_HARMONIA_SDR1)
        rx = 0;
      else if (rx_pmt == PMT_HARMONIA_SDR2)
        rx = 1;
      else if (rx_pmt == PMT_HARMONIA_SDR3)
        rx = 2;
      else
        return;

      pmt::pmt_t sdr_syms[num_platforms] = {PMT_HARMONIA_SDR1,
                                            PMT_HARMONIA_SDR2,
                                            PMT_HARMONIA_SDR3};
      pmt::pmt_t phase_syms[num_platforms] = {PMT_HARMONIA_P_SDR1,
                                              PMT_HARMONIA_P_SDR2,
                                              PMT_HARMONIA_P_SDR3};

      // Sort incoming data into their designated time/phase matrices
      for (int tx = 0; tx < num_platforms; ++tx)
      {
        // Skip TX = RX
        if (tx == rx)
          continue;

        // Time Matrix
        if (pmt::dict_has_key(d_meta, sdr_syms[tx]))
        {
          auto v = pmt::dict_ref(d_meta, sdr_syms[tx], pmt::PMT_NIL);
          if (pmt::is_f64vector(v))
          {
            auto tmp = pmt::f64vector_elements(v);
            if (!tmp.empty())
              time_matrix[rx][tx] = tmp[0];
          }
        }

        // Phase Matrix
        if (pmt::dict_has_key(d_meta, phase_syms[tx]))
        {
          auto v = pmt::dict_ref(d_meta, phase_syms[tx], pmt::PMT_NIL);
          if (pmt::is_f64vector(v))
          {
            auto tmp = pmt::f64vector_elements(v);
            if (!tmp.empty())
              phase_matrix[rx][tx] = tmp[0];
          }
        }
      }

      // Check if all values have been received
      check_time[rx] = true;
      check_phase[rx] = true;
      bool all_time = std::all_of(check_time.begin(), check_time.end(),
                                  [](bool b)
                                  { return b; });
      bool all_phase = std::all_of(check_phase.begin(), check_phase.end(),
                                   [](bool b)
                                   { return b; });
      if (!(all_time && all_phase))
        return;

      // AF time and phase matrix
      af::array m_est = to_af_array(time_matrix);
      af_print(m_est);
      af::array phase_est = to_af_array(phase_matrix);
      // af_print(phase_est);
      af::array phase_est_wrap = wrapToPi_af(phase_est);

      // AF TOF and PHI Estimates
      af::array TOF_est = (m_est + m_est.T()) / 2.0;
      af::array PHI_est = (m_est - m_est.T()) / 2.0;
      af::array R_est = TOF_est * 299792458.0;
      af_print(R_est);

      // af_print(TOF_est);
      // af_print(PHI_est);

      // ================================ CLOCK BIAS ESTIMATION ==================================
      if (bias_status)
        cb_est = af::sum(PHI_est, 1) / num_platforms;
      // af_print(cb_est);

      ///////////////////// PRINT /////////////////////////
      // std::vector<double> host_phi(3);
      // cb_est.host(host_phi.data());
      // std::cout << std::fixed << std::setprecision(12)
      //           << "Clock bias estimates: [";
      // for (int i = 0; i < N; ++i)
      // {
      //   std::cout << host_phi[i]
      //             << (i + 1 < N ? ", " : "]\n");
      // }
      /////////////////////////////////////////////////////

      // =================================== PHASE ESTIMATION ====================================
      if (phase_status)
      {
        af::array gamma = phase_est + (2 * M_PI * center_freq * m_est);
        af::array gamma_cal_wrap = wrapToPi_af(gamma);

        // Extract off-diagonal entries of gamma_cal_wrap into y_err
        af::array row_base = af::iota(af::dim4(num_platforms, 1), af::dim4(1, 1), u32);
        af::array rowIdx = af::tile(row_base, af::dim4(1, num_platforms));
        af::array col_base = af::iota(af::dim4(1, num_platforms), af::dim4(1, 1), u32);
        af::array colIdx = af::tile(col_base, af::dim4(num_platforms, 1));
        af::array mask = rowIdx != colIdx;
        af::array y_err = gamma_cal_wrap(mask);

        // Append zero to y_err
        af::array extra_zero = af::constant(0.0, 1, 1, gamma_cal_wrap.type());
        y_err = af::join(0, y_err, extra_zero);
        // af_print(y_err);

        // Build A_gamma:
        int base_rows = num_platforms * (num_platforms - 1);
        int ntrans = 2 * num_platforms;

        af::array A_gamma = af::constant(0.0, base_rows + 1, ntrans);
        int ind = 0;
        for (int jj = 0; jj < num_platforms; ++jj)
        {
          for (int ii = 0; ii < num_platforms; ++ii)
          {
            if (ii == jj)
              continue;
            A_gamma(ind, jj) = 1.0;
            A_gamma(ind, num_platforms + ii) = -1.0;
            ind++;
          }
        }
        // Gamma_TX = 1 Assumption
        A_gamma(base_rows, 0) = 1.0;
        // af_print(A_gamma);

        // Solving for Carrier Phases
        af::array x_est = af::constant(0.0, ntrans, f64);
        int loops = ntrans / num_platforms;
        auto twoNonzeroIndices = [&](const af::array &v)
        {
          af::array idx = af::where(v != 0);
          idx = idx.as(s32);
          int i0 = idx(0).scalar<int>();
          int i1 = idx(1).scalar<int>();
          return std::make_pair(i0, i1);
        };

        // Solving for RX Carrier Phases
        std::vector<std::pair<int, int>> ind_val_tx(loops);
        for (int ii = 0; ii < loops; ++ii)
        {
          auto [c0, c1] = twoNonzeroIndices(A_gamma.row(ii));
          ind_val_tx[ii] = {c0, c1};
          x_est(num_platforms + ii + 1) = -1.0 * y_err(ii);
        }

        // Solving for TX Carrier Phases
        for (int ii = 0; ii < loops; ++ii)
        {
          int tx_col = ind_val_tx[ii].second;
          auto [r0, r1] = twoNonzeroIndices(A_gamma.col(tx_col));
          auto [c0, c1] = twoNonzeroIndices(A_gamma.row(r1));
          x_est(c0) = y_err(r1) + x_est(tx_col);
        }

        // Solving for RX=1 Carrier Phase
        auto [r0, r1] = twoNonzeroIndices(A_gamma.col(num_platforms));
        auto [c0, c1] = twoNonzeroIndices(A_gamma.row(r1));
        x_est(num_platforms) = -1.0 * (y_err(r1) - x_est(c0));
        // af_print(x_est);

        // Compute y_est = A_gamma * x_est
        af::array y_est = af::matmul(A_gamma.as(f64), x_est);

        // Compute gamma_wrap
        af::array y_est_d = y_est.as(f64);
        af::array y_err_d = y_err.as(f64);
        af::array diff = (y_est_d - y_err_d) / (2.0 * M_PI);
        af::array rounded = af::round(diff);
        af::array gamma_wrap = rounded * (2.0 * M_PI);

        // Compute variance matrix
        double var_f = 3.0 /
                       (2.0 * std::pow(af::Pi, 2.0) *
                        std::pow(pulse_width, 3.0) *
                        samp_rate *
                        std::pow(10.0, (SNR / 10.0)) *
                        (1.0 - std::pow(1.0 / (pulse_width * samp_rate), 2.0)));

        af::array diag_vals = af::join(
            0,
            af::constant(var_f, 2.0 * num_platforms, f64),
            af::constant(1e-12, 1.0, f64));
        af::array C_err = af::diag(diag_vals, 0, false);

        // Cast A_gamma to double
        af::array A_gamma_d = A_gamma.as(f64);

        // Solve the system
        af::array Cinv = af::inverse(C_err);
        af::array At_Cinv = af::matmul(A_gamma_d.T(), Cinv);
        af::array mid = af::matmul(At_Cinv, A_gamma_d);
        af::array inv_mid = af::inverse(mid);
        af::array rhs = af::matmul(At_Cinv, (y_err_d + gamma_wrap));
        af::array x_gamma = af::matmul(inv_mid, rhs);
        x_gamma_est = wrapToPi_af(x_gamma);
      }
      // af_print(x_gamma_est);

      //////////////////////////////////////////////////////////////////////////////
      // double r10 = R_est(1, 0).scalar<double>();
      // double r20 = R_est(2, 0).scalar<double>();
      // double r21 = R_est(2, 1).scalar<double>();
      // std::vector<double> ranges = {r10, r20, r21};
      // pmt::pmt_t pmt_ranges = pmt::init_f64vector(ranges.size(), ranges.data());
      // meta = pmt::dict_add(meta, pmt::intern("my_range_estimates"), pmt_ranges);
      //////////////////////////////////////////////////////////////////////////////

      // =================================== OUTPUT ESTIMATES ====================================
      if (bias_status)
      {
        std::vector<double> x_host(cb_est.elements());
        cb_est.host(x_host.data());
        meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR1, pmt::from_double(x_host[0]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR2, pmt::from_double(x_host[1]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR3, pmt::from_double(x_host[2]));
        meta = pmt::dict_add(meta, pmt::intern("clock_bias_enable"), pmt::PMT_T);
      }

      if (phase_status)
      {
        std::vector<double> x_gamma_host(x_gamma_est.elements());
        x_gamma_est.host(x_gamma_host.data());
        meta = pmt::dict_add(meta, PMT_HARMONIA_CP_TX_SDR1, pmt::from_double(x_gamma_host[0]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CP_TX_SDR2, pmt::from_double(x_gamma_host[1]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CP_TX_SDR3, pmt::from_double(x_gamma_host[2]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CP_RX_SDR1, pmt::from_double(x_gamma_host[3]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CP_RX_SDR2, pmt::from_double(x_gamma_host[4]));
        meta = pmt::dict_add(meta, PMT_HARMONIA_CP_RX_SDR3, pmt::from_double(x_gamma_host[5]));
        meta = pmt::dict_add(meta, pmt::intern("carrier_phase_enable"), pmt::PMT_T);
      }

      // Output Meta Data
      message_port_pub(PMT_HARMONIA_OUT, meta);

      // pmt::pmt_t empty_vec = pmt::make_u8vector(0, 0);
      // message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, empty_vec));
    }

  } /* namespace harmonia */
} /* namespace gr */
