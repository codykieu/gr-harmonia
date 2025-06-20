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
      message_port_register_in(PMT_HARMONIA_CBP_IN);
      message_port_register_out(PMT_HARMONIA_OUT);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CD_IN, [this](pmt::pmt_t msg)
                      { handle_cd_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CBP_IN, [this](pmt::pmt_t msg)
                      { handle_cbp_msg(msg); });
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

    void compensation_impl::handle_cbp_msg(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      // Extract Clock Bias Values
      phi[0] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR1, pmt::from_double(1.0)));
      phi[1] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR2, pmt::from_double(1.0)));
      phi[2] = pmt::to_double(pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR3, pmt::from_double(1.0)));

      phi_ready = true;
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

      if (!pmt::is_c32vector(samples))
      {
        std::cout << "Samples are not c32vector\n";
        return;
      }

      size_t len = 0;
      const gr_complex *in_ptr = pmt::c32vector_elements(samples, len);
      if (len == 0 || !in_ptr)
      {
        std::cout << "Vector empty or null ptr\n";
        return;
      }

      // Phase adjustment variables
      std::vector<gr_complex> host(len);
      const double constant = 2.0 * M_PI * center_freq;
      const double Ts = 1.0 / samp_rate;
      if (alpha_ready)
        alpha_hat = alpha[sdr_id - 1];
      else
        alpha_hat = 1.0;
      if (phi_ready)
        phi_hat = phi[sdr_id - 1];
      else
        phi_hat = 0.0;

      // Phase adjusting signal
      for (size_t n = 0; n < len; ++n)
      {
        double tau = n * Ts;
        double phase = constant * ((alpha_hat - 1.0) * tau + phi_hat);
        std::complex<float> phcorr = std::exp(std::complex<float>(0.0f, float(phase)));
        host[n] = in_ptr[n] * phcorr;
      }

      // Export data
      pmt::pmt_t out_vec = pmt::init_c32vector(len, host.data());
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, out_vec));
    }

  } /* namespace harmonia */
} /* namespace gr */
