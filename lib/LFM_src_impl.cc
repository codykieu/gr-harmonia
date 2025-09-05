/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "LFM_src_impl.h"
#include <gnuradio/io_signature.h>

const double c = 299792458.0;

namespace gr
{
  namespace harmonia
  {

    using output_type = gr_complex;
    LFM_src::sptr LFM_src::make(
        double bandwidth, double start_freq, double center_freq, double pulse_width, double pulse_width2, double samp_rate, double prf, int zeropad, int sdr_id)
    {
      return gnuradio::make_block_sptr<LFM_src_impl>(
          bandwidth, start_freq, center_freq, pulse_width, pulse_width2, samp_rate, prf, zeropad, sdr_id);
    }

    af::array LFM(double bandwidth, double start_freq, double center_freq, double pulse_width,
                  double samp_rate, double prf, int zeropad, double alpha_hat = 1.0, double phi_hat = 0.0, double R_hat = 0.0, double gamma_hat = 0.0)
    {
      // Sample interval
      double ts = 1 / samp_rate;
      size_t n_samp_pulse = round(samp_rate * pulse_width);
      af::array t = (af::range(af::dim4(n_samp_pulse), -1, f64) * ts);

      // LFM Phase
      af::array phase =
          af::Im * 2 * M_PI *
          (start_freq * ((t) / alpha_hat) + bandwidth / (2 * pulse_width) * af::pow(((t) / alpha_hat), 2));

      // Clock Drift Phase Correction
      af::array cd_correction = af::Im * (2 * M_PI * center_freq * ((1 / alpha_hat) - 1) * t);

      // Clock Bias Phase Correction
      af::cdouble cb_scalar(0.0, 2 * M_PI * center_freq * phi_hat);
      af::array cb_correction = af::constant(cb_scalar, 1, 1);

      // Range Phase Correction
      af::cdouble range_scalar(0.0, 2 * M_PI * center_freq * R_hat/c);
      af::array range_correction = af::constant(range_scalar, 1, 1);

      // Carrier Phase Correction
      af::cdouble phase_scalar(0.0, gamma_hat);
      af::array phase_correction = af::constant(phase_scalar, 1, 1);

      // Create LFM
      af::array x = af::exp(phase + cd_correction - cb_correction - phase_correction + range_correction);

      // Zero-pad to PRF
      if (prf > 0)
      {
        size_t n_samp_pri = round(samp_rate / prf);
        size_t n_pad = n_samp_pri - n_samp_pulse;
        af::array zeros = af::constant(af::cdouble(0, 0), af::dim4(n_pad));
        x = af::join(0, x, zeros);
      }

      if (zeropad > 0)
      {
        size_t n_pad = zeropad / 2;
        af::array zeros = af::constant(af::cdouble(0, 0), af::dim4(n_pad));
        x = af::join(0, zeros, x);
        x = af::join(0, x, zeros);
      }

      return x;
    }

    /*
     * The private constructor
     */
    LFM_src_impl::LFM_src_impl(
        double bandwidth, double start_freq, double center_freq, double pulse_width, double pulse_width2, double samp_rate, double prf, int zeropad, int sdr_id)
        : gr::block(
              "LFM_src", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          bandwidth(bandwidth),
          start_freq(start_freq),
          center_freq(center_freq),
          pulse_width(pulse_width),
          pulse_width2(pulse_width2),
          samp_rate(samp_rate),
          prf(prf),
          zeropad(zeropad),
          sdr_id(sdr_id)
    {
      cd_val = pmt::from_double(1.0);
      cb_val = pmt::from_double(0.0);
      cp_val = pmt::from_double(0.0);
      R_val = pmt::from_double(0.0);
      af::array waveform = LFM(bandwidth, start_freq, center_freq, pulse_width, samp_rate, prf, zeropad).as(c32);

      d_data = pmt::init_c32vector(
          waveform.elements(), reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

      message_port_register_in(PMT_HARMONIA_CD_IN);
      message_port_register_in(PMT_HARMONIA_CB_IN);
      message_port_register_in(PMT_HARMONIA_CP_IN);

      message_port_register_out(PMT_HARMONIA_OUT);

      set_msg_handler(PMT_HARMONIA_CD_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CB_IN, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
      set_msg_handler(PMT_HARMONIA_CP_IN, [this](pmt::pmt_t msg)
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
        // Extrract Clock Drift Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_SDR1))
        {
          cd_val = pmt::dict_ref(msg, PMT_HARMONIA_SDR1, cd_val);
          meta = pmt::dict_add(meta, PMT_HARMONIA_SDR1, cd_val);
        }
        // Extract Clock Bias Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CB_SDR1))
        {
          cb_val = pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR1, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR1, cb_val);
        }
        // Extract Carrier Phase Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CP_TX_SDR1))
        {
          cp_val = pmt::dict_ref(msg, PMT_HARMONIA_CP_TX_SDR1, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CP_TX_SDR1, cp_val);
        }
        break;
      }

      case 2:
      {
        // Extract Clock Drift Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_SDR2))
        {
          cd_val = pmt::dict_ref(msg, PMT_HARMONIA_SDR2, cd_val);
          meta = pmt::dict_add(meta, PMT_HARMONIA_SDR2, cd_val);
        }
        // Extract Clock Bias Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CB_SDR2))
        {
          cb_val = pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR2, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR2, cb_val);
        }
        // Extract Carrier Phase Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CP_TX_SDR2))
        {
          cp_val = pmt::dict_ref(msg, PMT_HARMONIA_CP_TX_SDR2, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CP_TX_SDR2, cp_val);
        }
        // Extract Range
        if (pmt::dict_has_key(msg, PMT_HARMONIA_R_SDR12))
        {
          R_val = pmt::dict_ref(msg, PMT_HARMONIA_R_SDR12, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_R_SDR12, R_val);
        }
        break;
      }

      case 3:
      {
        // Extract Clock Drift Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_SDR3))
        {
          cd_val = pmt::dict_ref(msg, PMT_HARMONIA_SDR3, cd_val);
          meta = pmt::dict_add(meta, PMT_HARMONIA_SDR3, cd_val);
        }
        // Extract Clock Bias Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CB_SDR3))
        {
          cb_val = pmt::dict_ref(msg, PMT_HARMONIA_CB_SDR3, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CB_SDR3, cb_val);
        }
        // Extract Carrier Phase Value
        if (pmt::dict_has_key(msg, PMT_HARMONIA_CP_TX_SDR3))
        {
          cp_val = pmt::dict_ref(msg, PMT_HARMONIA_CP_TX_SDR3, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_CP_TX_SDR3, cp_val);
        }
        // Extract Range
        if (pmt::dict_has_key(msg, PMT_HARMONIA_R_SDR13))
        {
          R_val = pmt::dict_ref(msg, PMT_HARMONIA_R_SDR13, pmt::PMT_NIL);
          meta = pmt::dict_add(meta, PMT_HARMONIA_R_SDR13, R_val);
        }
        break;
      }
      }

      // Convert Clock Drift, Clock Bias, Carrier Phase PMTs to Doubles
      alpha_hat = pmt::to_double(cd_val);
      phi_hat = pmt::to_double(cb_val);
      gamma_hat = pmt::to_double(cp_val);
      R_hat = pmt::to_double(R_val);
      // GR_LOG_INFO(d_logger, "SDR: " + std::to_string(sdr_id) + "Clock Drift: " + std::to_string(alpha_hat) + "Clock Bias: " + std::to_string(phi_hat));

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

      // Attach Carrier Phase flag
      pmt::pmt_t phase_flag = pmt::dict_ref(msg, pmt::intern("carrier_phase_enable"), pmt::PMT_F);
      if (pmt::equal(phase_flag, pmt::PMT_T))
      {
        meta = pmt::dict_add(meta, pmt::intern("carrier_phase_enable"), pmt::PMT_T);
      }

      // Label Waveform
      meta = pmt::dict_add(meta, PMT_HARMONIA_LABEL, pmt::intern("LFM"));

      // Generate waveform with corrected drift/bias
      if ((pmt::equal(phase_flag, pmt::PMT_T)) || pmt::equal(bias_flag, pmt::PMT_T))
      {
        waveform = LFM(bandwidth, start_freq, center_freq, pulse_width2,
                       samp_rate, prf, zeropad, alpha_hat, phi_hat, R_hat, gamma_hat)
                       .as(c32);
      }
      else
      {
        waveform = LFM(bandwidth, start_freq, center_freq, pulse_width,
                       samp_rate, prf, zeropad, alpha_hat, phi_hat, R_hat, gamma_hat)
                       .as(c32);
      }

      d_data = pmt::init_c32vector(
          waveform.elements(),
          reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

      // Send updated message
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(meta, d_data));
      meta = pmt::make_dict();
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
