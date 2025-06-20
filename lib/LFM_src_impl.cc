/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "LFM_src_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    using output_type = gr_complex;
    LFM_src::sptr LFM_src::make(
        double bandwidth, double start_freq, double center_freq, double pulse_width, double samp_rate, double prf, int sdr_id)
    {
      return gnuradio::make_block_sptr<LFM_src_impl>(
          bandwidth, start_freq, center_freq, pulse_width, samp_rate, prf, sdr_id);
    }

    af::array LFM(double bandwidth, double start_freq, double center_freq, double pulse_width,
                  double samp_rate, double prf, double alpha_hat = 1.0, double phi_hat = 0.0)
    {
      // Sample interval
      double ts = 1 / samp_rate;
      size_t n_samp_pulse = round(samp_rate * pulse_width);
      af::array t = (af::range(af::dim4(n_samp_pulse), -1, f64) * ts);
      af::array phase = 
          af::Im * 2 * M_PI *
          (start_freq * ((t - phi_hat) / alpha_hat) + bandwidth / (2 * pulse_width) * af::pow(((t - phi_hat) / alpha_hat), 2));
      af::array cd_correction = af::Im * (2 * M_PI * center_freq * ((1 / alpha_hat) - 1) * t);
      
      af::cdouble cb_scalar(0.0, 2 * M_PI * center_freq * phi_hat/alpha_hat);
      af::array cb_correction = af::constant(cb_scalar, 1, 1);

      af::array x = af::exp(phase + cd_correction - cb_correction);

      // Zero-pad to PRF
      if (prf > 0)
      {
        size_t n_samp_pri = round(samp_rate / prf);
        size_t n_pad = n_samp_pri - n_samp_pulse;
        af::array zeros = af::constant(af::cdouble(0, 0), af::dim4(n_pad));
        x = af::join(0, x, zeros);
      }

      return x;
    }

    /*
     * The private constructor
     */
    LFM_src_impl::LFM_src_impl(
        double bandwidth, double start_freq, double center_freq, double pulse_width, double samp_rate, double prf, int sdr_id)
        : gr::block(
              "LFM_src", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          bandwidth(bandwidth),
          start_freq(start_freq),
          center_freq(center_freq),
          pulse_width(pulse_width),
          samp_rate(samp_rate),
          prf(prf),
          sdr_id(sdr_id)
    {
      cd_val = pmt::from_double(1.0);
      cb_val = pmt::from_double(0.0);

      af::array waveform = LFM(bandwidth, start_freq, center_freq, pulse_width, samp_rate, prf).as(c32);

      d_data = pmt::init_c32vector(
          waveform.elements(), reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

      message_port_register_in(PMT_HARMONIA_CD_IN);
      message_port_register_in(PMT_HARMONIA_CBP_IN);
      message_port_register_out(PMT_HARMONIA_OUT);

      set_msg_handler(PMT_HARMONIA_CD_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CBP_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    LFM_src_impl::~LFM_src_impl() {}

    bool LFM_src_impl::start()
    {
      // Send a PDU containing the waveform and its metadata
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, d_data));

      return block::start();
    }

    void LFM_src_impl::handle_msg(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      // Select the right drift and bias value based on SDR ID
      switch (sdr_id)
      {
      case 1:
      {
        if (pmt::dict_has_key(msg, PMT_HARMONIA_SDR1))
        {
          // cd_val = pmt::from_double(1.0);
          cd_val = pmt::dict_ref(msg, PMT_HARMONIA_SDR1, cd_val);
          meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, cd_val);
        }

        if (pmt::dict_has_key(msg, PMT_HARMONIA_CB_SDR1))
        {
          cb_val = pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR1, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR1, cb_val);
        }
        break;
      }

      case 2:
      {
        if (pmt::dict_has_key(msg, PMT_HARMONIA_SDR2))
        {
          // cd_val = pmt::from_double(1.0);
          cd_val = pmt::dict_ref(msg, PMT_HARMONIA_SDR2, cd_val);
          meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, cd_val);
        }

        if (pmt::dict_has_key(msg, PMT_HARMONIA_CB_SDR2))
        {
          cb_val = pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR2, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR2, cb_val);
        }
        break;
      }

      case 3:
      {
        if (pmt::dict_has_key(msg, PMT_HARMONIA_SDR3))
        {
          // cd_val = pmt::from_double(1.0);
          cd_val = pmt::dict_ref(msg, PMT_HARMONIA_SDR3, cd_val);
          meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, cd_val);
        }
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CB_SDR3))
        {
          cb_val = pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR3, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR3, cb_val);
        }
        break;
      }
      }

      // Validate the drift value
      if (!pmt::is_number(cd_val))
      {
        GR_LOG_ERROR(d_logger, "Clock drift value missing or invalid");
        return;
      }

      // Use the drift/bias value
      alpha_hat = pmt::to_double(cd_val);
      phi_hat = pmt::to_double(cb_val);

      // GR_LOG_INFO(d_logger, "SDR: " + std::to_string(sdr_id) + "Clock Drift: " + std::to_string(alpha_hat) + "Clock Bias: " + std::to_string(phi_hat));

      // Generate waveform with corrected drift/bias
      af::array waveform = LFM(bandwidth, start_freq, center_freq, pulse_width,
                               samp_rate, prf, alpha_hat, phi_hat)
                               .as(c32);

      d_data = pmt::init_c32vector(
          waveform.elements(),
          reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

      // Attatch Clock Drift flag
      pmt::pmt_t drift_flag = pmt::dict_ref(msg, pmt::intern("clock_drift_enable"), pmt::PMT_F);
      if (pmt::equal(drift_flag, pmt::PMT_T))
      {
        meta = pmt::dict_add(meta, pmt::intern("clock_drift_enable"), pmt::PMT_T);
      }

      // Attach Clock Bias flag
      pmt::pmt_t bias_flag = pmt::dict_ref(msg, pmt::intern("clock_bias_enable"), pmt::PMT_F);
      if (pmt::equal(bias_flag, pmt::PMT_T))
      {
        meta = pmt::dict_add(meta, pmt::intern("clock_bias_enable"), pmt::PMT_T);
      }

      meta = pmt::dict_add(meta, PMT_HARMONIA_LABEL, pmt::intern("LFM"));

      // Send updated message
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, d_data));
    }

    void LFM_src_impl::init_meta_dict(const std::string &bandwidth_key,
                                      const std::string &duration_key,
                                      const std::string &sample_rate_key,
                                      const std::string &label_key,
                                      const std::string &prf_key)
    {
      this->bandwidth_key = pmt::intern(bandwidth_key);
      this->duration_key = pmt::intern(duration_key);
      this->sample_rate_key = pmt::intern(sample_rate_key);
      this->label_key = pmt::intern(label_key);
      this->prf_key = pmt::intern(prf_key);

      meta = pmt::make_dict();
      meta = pmt::dict_add(meta, this->bandwidth_key, pmt::from_double(bandwidth));
      meta = pmt::dict_add(meta, this->duration_key, pmt::from_double(pulse_width));
      meta = pmt::dict_add(meta, this->sample_rate_key, pmt::from_double(samp_rate));
      meta = pmt::dict_add(meta, this->label_key, pmt::intern("LFM"));
      meta = pmt::dict_add(meta, this->prf_key, pmt::from_double(prf));
    }

  } /* namespace harmonia */
} /* namespace gr */
