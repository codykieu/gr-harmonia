/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "QPSK_mod_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    QPSK_mod::sptr QPSK_mod::make(int sps)
    {
      return gnuradio::make_block_sptr<QPSK_mod_impl>(sps);
    }

    /*
     * The private constructor
     */
    QPSK_mod_impl::QPSK_mod_impl(int sps)
        : gr::block("QPSK_mod",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          d_sps(sps),
          d_phase(0)
    {
      d_in_port = PMT_HARMONIA_IN;
      d_out_port = PMT_HARMONIA_OUT;
      message_port_register_in(d_in_port);
      message_port_register_out(d_out_port);
      set_msg_handler(d_in_port, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    QPSK_mod_impl::~QPSK_mod_impl() {}

    void QPSK_mod_impl::handle_msg(pmt::pmt_t msg)
    {
      std::cout << "[QPSK_mod] handle_msg called" << std::endl;
      if (!pmt::is_pair(msg))
      {
        std::cout << "[QPSK_mod] Not a pair" << std::endl;
        return;
      }
      auto meta = pmt::car(msg);
      auto vec = pmt::cdr(msg);
      if (!pmt::is_f32vector(vec) || pmt::length(vec) != 2)
      {
        std::cout << "[QPSK_mod] Expecting f32vector length 2" << std::endl;
        return;
      }

      // extract floats
      size_t len;
      const float *vals = pmt::f32vector_elements(vec, len);

      // serialize to bits
      std::vector<uint8_t> bits;
      bits.reserve(64);
      for (int i = 0; i < 2; i++)
      {
        uint32_t u = 0;
        std::memcpy(&u, &vals[i], sizeof(u));
        for (int b = 31; b >= 0; b--)
        {
          bits.push_back((u >> b) & 0x1);
        }
      }

      // QPSK symbol mapping
      std::vector<std::complex<float>> waveform;
      waveform.reserve((bits.size() / 2) * d_sps);
      const float delta = 0.0f; // carrier rotation per sample (disabled)

      for (size_t i = 0; i + 1 < bits.size(); i += 2)
      {
        uint8_t b0 = bits[i], b1 = bits[i + 1];
        float phase_offset = 0.0f;
        switch ((b0 << 1) | b1)
        {
        case 0:
          phase_offset = M_PI / 4.0f;
          break;
        case 1:
          phase_offset = 3 * M_PI / 4.0f;
          break;
        case 2:
          phase_offset = -3 * M_PI / 4.0f;
          break;
        case 3:
          phase_offset = -M_PI / 4.0f;
          break;
        }

        for (int k = 0; k < d_sps; k++)
        {
          float phi = d_phase + phase_offset;
          waveform.emplace_back(std::polar(1.0f, phi));
          d_phase += delta;

          if (d_phase > M_PI)
            d_phase -= 2 * M_PI;
          else if (d_phase < -M_PI)
            d_phase += 2 * M_PI;
        }
      }

      auto payload = pmt::init_c32vector(waveform.size(), waveform.data());
      message_port_pub(d_out_port, pmt::cons(meta, payload));
      std::cout << "[QPSK_mod] Published " << waveform.size() << " samples" << std::endl;
    }

  } /* namespace harmonia */
} /* namespace gr */
