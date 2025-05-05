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
        double frequency, double phase, double pulse_width, double samp_rate, double prf)
    {
      return gnuradio::make_block_sptr<single_tone_src_impl>(
          frequency, phase, pulse_width, samp_rate, prf);
    }

    // Single-tone Waveform Generator
    af::array single_tone(double frequency, double phase, double pulse_width,
                          double samp_rate, double prf)
    {
      double ts = 1 / samp_rate;
      size_t n_samp_pulse = round(samp_rate * pulse_width);
      af::array t = af::range(af::dim4(n_samp_pulse), -1, f64) * ts;
      af::array wave_phase = af::Im * (2 * M_PI * frequency * t + phase);
      af::array x = af::exp(wave_phase);

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
    single_tone_src_impl::single_tone_src_impl(
        double frequency, double phase, double pulse_width, double samp_rate, double prf)
        : gr::block(
              "single_tone_src", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          msg_port(PMT_HARMONIA_IN),
          out_port(PMT_HARMONIA_OUT),
          frequency(frequency),
          phase(phase),
          pulse_width(pulse_width),
          samp_rate(samp_rate),
          prf(prf)
    {
      af::array waveform =
          single_tone(frequency, phase, pulse_width, samp_rate, prf).as(c32);
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

    /**
     * @brief Send a PDU containing the waveform data and metadata
     *
     * @return true
     * @return false
     */
    bool single_tone_src_impl::start()
    {
      // Send a PDU containing the waveform and its metadata
      message_port_pub(out_port, pmt::cons(meta, d_data));

      return block::start();
    }

    void single_tone_src_impl::handle_msg(pmt::pmt_t msg)
    {
      // TODO: Handle dictionary inputs for changing many parameters at once
      pmt::pmt_t key = pmt::car(msg);
      pmt::pmt_t value = pmt::cdr(msg);

      if (pmt::equal(key, frequency_key))
      {
        frequency = pmt::to_double(value);
        meta = pmt::dict_add(meta, frequency_key, pmt::from_double(frequency));
      }
      else if (pmt::equal(key, phase_key))
      {
        phase = pmt::to_double(value);
        meta = pmt::dict_add(meta, phase_key, pmt::from_double(phase));
      }
      else if (pmt::equal(key, duration_key))
      {
        pulse_width = pmt::to_double(value);
        meta = pmt::dict_add(meta, duration_key, pmt::from_double(pulse_width));
      }
      else if (pmt::equal(key, sample_rate_key))
      {
        samp_rate = pmt::to_double(value);
        meta = pmt::dict_add(meta, sample_rate_key, pmt::from_double(samp_rate));
      }
      else if (pmt::equal(key, prf_key))
      {
        prf = pmt::to_double(value);
        meta = pmt::dict_add(meta, prf_key, pmt::from_double(prf));
      }

      // Create the new waveform vector and emit it as a PDU
      af::array waveform =
          single_tone(frequency, phase, pulse_width, samp_rate, prf).as(c32);
      d_data = pmt::init_c32vector(
          waveform.elements(), reinterpret_cast<gr_complex *>(waveform.host<af::cfloat>()));
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
