/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "single_tone_src_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    using output_type = gr_complex;
    single_tone_src::sptr single_tone_src::make(
        double frequency, double center_freq, double phase, double pulse_width, double samp_rate, double prf, int max_buffer_size, int sdr_id)
    {
      return gnuradio::make_block_sptr<single_tone_src_impl>(
          frequency, center_freq, phase, pulse_width, samp_rate, prf, max_buffer_size, sdr_id);
    }

    af::array single_tone(double frequency, double center_freq, double phase, double pulse_width,
                          double samp_rate, double prf, int max_buffer_size, double alpha_hat = 1.0)
    {
      double ts = 1 / samp_rate;
      size_t n_samp_pulse = round(samp_rate * pulse_width);

      // Time vector
      af::array t = af::range(af::dim4(n_samp_pulse), -1, f64) * ts;

      // Waveform + Correction
      af::array phase_term = af::Im * (2 * M_PI * frequency * (t / alpha_hat) + phase);
      af::array correction_term = af::Im * (2 * M_PI * center_freq * ((1 / alpha_hat) - 1) * t);
      af::array x = af::exp(phase_term + correction_term);

      // PRF zero padding
      if (prf > 0)
      {
        size_t n_samp_pri = round(samp_rate / prf);
        size_t n_pad = n_samp_pri - n_samp_pulse;
        af::array zeros = af::constant(af::cdouble(0, 0), af::dim4(n_pad));
        x = af::join(0, x, zeros);
      }

      // // Buffer correction
      // size_t remainder = x.elements() % max_buffer_size;
      // if (remainder != 0)
      // {
      //   size_t pad = max_buffer_size - remainder;
      //   af::array pad_arr = af::constant(af::cdouble(0, 0), af::dim4(pad));
      //   x = af::join(0, x, pad_arr);
      // }

      return x;
    }

    /*
     * The private constructor
     */
    single_tone_src_impl::single_tone_src_impl(
        double frequency, double center_freq, double phase, double pulse_width, double samp_rate, double prf, int max_buffer_size, int sdr_id)
        : gr::block(
              "single_tone_src", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          msg_port(PMT_HARMONIA_IN),
          out_port(PMT_HARMONIA_OUT),
          frequency(frequency),
          center_freq(center_freq),
          phase(phase),
          pulse_width(pulse_width),
          samp_rate(samp_rate),
          prf(prf),
          max_buffer_size(max_buffer_size),
          sdr_id(sdr_id)
    {
      af::array waveform = single_tone(frequency, center_freq, phase, pulse_width, samp_rate, prf, max_buffer_size).as(c32);

      d_data = pmt::init_c32vector(
          waveform.elements(), reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

      message_port_register_in(msg_port);
      message_port_register_out(out_port);

      set_msg_handler(msg_port, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    single_tone_src_impl::~single_tone_src_impl() {}

    bool single_tone_src_impl::start()
    {
      // Send a PDU containing the waveform and its metadata
      message_port_pub(out_port, pmt::cons(meta, d_data));

      return block::start();
    }

    // void single_tone_src_impl::handle_msg(pmt::pmt_t msg)
    // {
    //   // TODO: Handle dictionary inputs for changing many parameters at once
    //   pmt::pmt_t key = pmt::car(msg);
    //   pmt::pmt_t value = pmt::cdr(msg);

    //   // Default alpha_hat
    //   double alpha_hat = 1.0;

    //   pmt::pmt_t val;
    //   if (sdr_id == 1)
    //   {
    //     val = pmt::dict_ref(msg, PMT_HARMONIA_SDR1, pmt::PMT_NIL);
    //     // val = pmt::form_double(1.0);
    //     // GR_LOG_ERROR(d_logger, "Using for new waveform:" + std::to_string(sdr_id));
    //   }
    //   else if (sdr_id == 2)
    //   {
    //     val = pmt::dict_ref(msg, PMT_HARMONIA_SDR2, pmt::PMT_NIL);
    //     // val = pmt::from_double(1.00001);
    //     // GR_LOG_ERROR(d_logger, "Using for new waveform:" + std::to_string(sdr_id));
    //   }
    //   else if (sdr_id == 3)
    //   {
    //     val = pmt::dict_ref(msg, PMT_HARMONIA_SDR3, pmt::PMT_NIL);
    //   }
    //   else
    //   {
    //     GR_LOG_ERROR(d_logger, "Unknown SDR ID");
    //     return;
    //   }

    //   // Check if clock drift value is valid
    //   if (!pmt::is_number(val))
    //   {
    //     GR_LOG_ERROR(d_logger, "Clock drift not found or invalid for this SDR ID");
    //     return;
    //   }

    //   alpha_hat = pmt::to_double(val);
    //   // GR_LOG_ERROR(d_logger, "ALPHA_HAT" + std::to_string(alpha_hat));

    //   // Create the new waveform vector and emit it as a PDU
    //   af::array waveform =
    //       single_tone(frequency, center_freq, phase, pulse_width, samp_rate, prf, max_buffer_size, alpha_hat).as(c32);
    //   d_data = pmt::init_c32vector(
    //       waveform.elements(), reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

    //   message_port_pub(out_port, pmt::cons(meta, d_data));
    // }

    void single_tone_src_impl::handle_msg(pmt::pmt_t msg)
    {
      // Validate message is a PDU
      if (!pmt::is_pair(msg))
      {
        GR_LOG_ERROR(d_logger, "Expected message to be a PDU");
        return;
      }

      // Extract meta and data from the PDU
      pmt::pmt_t meta_in = pmt::car(msg);

      // Default alpha_hat
      double alpha_hat = 1.0;
      pmt::pmt_t val;

      // Select the right drift value based on SDR ID
      if (sdr_id == 1)
      {
        // val = pmt::dict_ref(msg, PMT_HARMONIA_SDR1, pmt::PMT_NIL);
        val = pmt::from_double(1.0);
      }
      else if (sdr_id == 2)
      {
        // val = pmt::dict_ref(msg, PMT_HARMONIA_SDR2, pmt::PMT_NIL);
        val = pmt::from_double(1.00001);

      }
      else if (sdr_id == 3)
      {
        val = pmt::dict_ref(msg, PMT_HARMONIA_SDR3, pmt::PMT_NIL);
      }
      else
      {
        GR_LOG_ERROR(d_logger, "Unknown SDR ID");
        return;
      }

      // Validate the drift value
      if (!pmt::is_number(val))
      {
        GR_LOG_ERROR(d_logger, "Clock drift value missing or invalid");
        return;
      }

      // Use the drift value
      alpha_hat = pmt::to_double(val);

      // Generate waveform with corrected drift
      af::array waveform = single_tone(
                               frequency, center_freq, phase, pulse_width,
                               samp_rate, prf, max_buffer_size, alpha_hat)
                               .as(c32);

      d_data = pmt::init_c32vector(
          waveform.elements(),
          reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));

      // Update outgoing metadata with original + SDR-specific value
      if (sdr_id == 1)
        meta = pmt::dict_add(msg, PMT_HARMONIA_SDR1, val);
      else if (sdr_id == 2)
        meta = pmt::dict_add(msg, PMT_HARMONIA_SDR2, val);
      else if (sdr_id == 3)
        meta = pmt::dict_add(msg, PMT_HARMONIA_SDR3, val);

      // Re-attach clock drift flag if it existed
      pmt::pmt_t drift_flag = pmt::dict_ref(msg, pmt::intern("clock_drift_enable"), pmt::PMT_F);
      if (pmt::equal(drift_flag, pmt::PMT_T))
      {
        meta = pmt::dict_add(meta, pmt::intern("clock_drift_enable"), pmt::PMT_T);
      }

      // Send updated message
      message_port_pub(out_port, pmt::cons(meta, d_data));
    }

    void single_tone_src_impl::init_meta_dict(const std::string &frequency_key,
                                              const std::string &phase_key,
                                              const std::string &duration_key,
                                              const std::string &sample_rate_key,
                                              const std::string &label_key,
                                              const std::string &prf_key)
    {
      this->frequency_key = pmt::intern(frequency_key);
      this->phase_key = pmt::intern(phase_key);
      this->duration_key = pmt::intern(duration_key);
      this->sample_rate_key = pmt::intern(sample_rate_key);
      this->label_key = pmt::intern(label_key);
      this->prf_key = pmt::intern(prf_key);

      meta = pmt::make_dict();
      meta = pmt::dict_add(meta, this->frequency_key, pmt::from_double(frequency));
      meta = pmt::dict_add(meta, this->phase_key, pmt::from_double(phase));
      meta = pmt::dict_add(meta, this->duration_key, pmt::from_double(pulse_width));
      meta = pmt::dict_add(meta, this->sample_rate_key, pmt::from_double(samp_rate));
      meta = pmt::dict_add(meta, this->label_key, pmt::intern("single_tone"));
      meta = pmt::dict_add(meta, this->prf_key, pmt::from_double(prf));
    }

  } /* namespace harmonia */
} /* namespace gr */