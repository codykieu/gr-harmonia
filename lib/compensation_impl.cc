/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "compensation_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    compensation::sptr compensation::make(double center_freq, double samp_rate, int sdr_id)
    {
      return gnuradio::make_block_sptr<compensation_impl>(center_freq, samp_rate, sdr_id);
    }

    /*
     * The private constructor
     */
    compensation_impl::compensation_impl(double center_freq, double samp_rate, int sdr_id)
        : gr::block("compensation",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          center_freq(center_freq),
          samp_rate(samp_rate),
          sdr_id(sdr_id)
    {

      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_in(PMT_HARMONIA_CD_IN);
      message_port_register_in(PMT_HARMONIA_CB_IN);
      message_port_register_in(PMT_HARMONIA_CP_IN);
      message_port_register_out(PMT_HARMONIA_OUT);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CD_IN, [this](pmt::pmt_t msg)
                      { handle_cd_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CB_IN, [this](pmt::pmt_t msg)
                      { handle_cb_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CP_IN, [this](pmt::pmt_t msg)
                      { handle_cp_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    compensation_impl::~compensation_impl() {}

    void compensation_impl::handle_cd_msg(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      // Extract Clock Drift Values
      alpha[0] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_SDR1, pmt::from_double(1.0)));
      alpha[1] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_SDR2, pmt::from_double(1.0)));
      alpha[2] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_SDR3, pmt::from_double(1.0)));
      alpha_ready = true;
    }

    void compensation_impl::handle_cb_msg(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      // Extract Clock Bias Values
      phi[0] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR1, pmt::from_double(0.0)));
      phi[1] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR2, pmt::from_double(0.0)));
      phi[2] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR3, pmt::from_double(0.0)));
      phi_ready = true;
    }

    void compensation_impl::handle_cp_msg(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      // Extract Carrier Phase Values
      gamma[0] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CP_RX_SDR1, pmt::from_double(0.0)));
      gamma[1] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CP_RX_SDR2, pmt::from_double(0.0)));
      gamma[2] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CP_RX_SDR3, pmt::from_double(0.0)));
      gamma_ready = true;
    }

    void compensation_impl::handle_msg(pmt::pmt_t msg)
    {
      // Extract data
      if (!pmt::is_pair(msg))
      {
        std::cout << "Not a PDU\n";
        return;
      }

      pmt::pmt_t meta = pmt::car(msg);
      pmt::pmt_t samples = pmt::cdr(msg);

      // Check for 'rx_time'
      pmt::pmt_t rx_time_err_key = pmt::intern("rx_error");
      if (pmt::dict_has_key(meta, rx_time_err_key))
      {
        pmt::pmt_t rx_time_err_val = pmt::dict_ref(meta, rx_time_err_key, pmt::PMT_NIL);
        if (pmt::is_real(rx_time_err_val))
        {
          t_delay = pmt::to_double(rx_time_err_val);
          // std::cout << "rx_time = " << t_delay << std::endl;
        }
      }

      if (!pmt::is_c32vector(samples))
      {
        std::cout << "Samples are not c32vector\n";
        return;
      }

      // Convert Data into AF vector
      size_t len = 0;
      const gr_complex *in_ptr = pmt::c32vector_elements(samples, len);
      af::array af_input = af::array(len, reinterpret_cast<const af::cfloat *>(in_ptr));

      if (len == 0 || !in_ptr)
      {
        std::cout << "Vector empty or null ptr\n";
        return;
      }

      // Update Clock Drift Value
      if (alpha_ready)
      {
        alpha_hat = alpha[sdr_id - 1];
      }
      else
        alpha_hat = 1.0;

      // Update Clock Bias Value
      if (phi_ready)
      {
        phi_hat = phi[sdr_id - 1];
      }
      else
        phi_hat = 0.0;

      // GR_LOG_INFO(d_logger, "SDR: " + std::to_string(sdr_id) + "Clock Drift: " + std::to_string(alpha_hat) + "Clock Bias: " + std::to_string(phi_hat));

      // Update Carrier Phase Value
      if (gamma_ready)
      {
        gamma_hat = gamma[sdr_id - 1];
      }
      else
        gamma_hat = 0.0;

      // FFT-base Fractional Delay
      af::array X = af::fft(af_input, len);
      X = ::plasma::fftshift(X, 0);
      af::array f = (-samp_rate / 2.0) + ((af::seq(0, len - 1)) * (samp_rate / len));
      af::array X_delay = X * af::exp(1.0 * af::Im * 2.0 * M_PI * f * (t_delay));
      X_delay = ::plasma::ifftshift(X_delay, 0);
      af::array x_delay = af::ifft(X_delay);

      // Output Data
      pmt::pmt_t vec = pmt::make_c32vector(len, gr_complex{0, 0});
      gr_complex *out_ptr = pmt::c32vector_writable_elements(vec, len);
      x_delay.host(reinterpret_cast<af::cfloat *>(out_ptr));

      // Phase adjustment variables
      std::vector<gr_complex> host(len);
      const double constant = 2.0 * M_PI * center_freq;
      const double Ts = 1.0 / samp_rate;

      // Phase adjusting signal
      for (size_t n = 0; n < len; ++n)
      {
        double tau = n * Ts;
        double phase = constant * ((alpha_hat - 1.0) * tau/alpha_hat + phi_hat) + gamma_hat;
        std::complex<float> phcorr = std::exp(std::complex<float>(0.0, float(phase)));
        host[n] = out_ptr[n] * phcorr;
      }

      // Export data
      pmt::pmt_t out_vec = pmt::init_c32vector(len, host.data());
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, out_vec));
    }

  } /* namespace harmonia */
} /* namespace gr */
