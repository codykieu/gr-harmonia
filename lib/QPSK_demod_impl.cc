/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "QPSK_demod_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    QPSK_demod::sptr QPSK_demod::make(int sps)
    {
      return gnuradio::make_block_sptr<QPSK_demod_impl>(sps);
    }

    /*
     * The private constructor
     */
    QPSK_demod_impl::QPSK_demod_impl(int sps)
        : gr::block("QPSK_demod",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          d_sps(sps)
    {
      d_in_port = PMT_HARMONIA_IN;
      d_out_port = PMT_HARMONIA_OUT;
      message_port_register_in(d_in_port);
      message_port_register_out(d_out_port);
      set_msg_handler(d_in_port,
                      [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    QPSK_demod_impl::~QPSK_demod_impl() {}

    void QPSK_demod_impl::handle_msg(pmt::pmt_t msg)
    {
      if (!pmt::is_pair(msg))
        return;

      pmt::pmt_t vec = pmt::cdr(msg);
      if (!pmt::is_c32vector(vec))
        return;

      size_t N;
      const gr_complex *in = pmt::c32vector_elements(vec, N);
      if (N < d_sps || (N % d_sps) != 0)
      {
          std::cout << "[QPSK_demod] Skipping: invalid length " << N << std::endl;
      }

      std::cout << "[QPSK_demod] Received message with " << N << " samples" << std::endl;

      const gr_complex ref_syms[4] = {
          gr_complex(1, 1), gr_complex(-1, 1),
          gr_complex(-1, -1), gr_complex(1, -1)};
      const bool bit_pairs[4][2] = {
          {0, 0}, {0, 1}, {1, 0}, {1, 1}};

      size_t num_syms = N / d_sps;
      for (size_t i = 0; i < num_syms; i++)
      {
        gr_complex s = in[i * d_sps + d_sps / 2];
        if (std::abs(s) != 0.0f)
          s /= std::abs(s); // normalize

        int best = 0;
        float best_dist = std::norm(s - ref_syms[0]);
        for (int j = 1; j < 4; ++j)
        {
          float d = std::norm(s - ref_syms[j]);
          if (d < best_dist)
          {
            best = j;
            best_dist = d;
          }
        }

        d_bits.push_back(bit_pairs[best][0]);
        d_bits.push_back(bit_pairs[best][1]);
      }

      while (d_bits.size() >= 64)
      {
        uint32_t u0 = 0, u1 = 0;
        for (int j = 0; j < 32; j++)
        {
          u0 = (u0 << 1) | static_cast<uint32_t>(d_bits.front());
          d_bits.pop_front();
        }
        for (int j = 0; j < 32; j++)
        {
          u1 = (u1 << 1) | static_cast<uint32_t>(d_bits.front());
          d_bits.pop_front();
        }

        float f0, f1;
        std::memcpy(&f0, &u0, sizeof(f0));
        std::memcpy(&f1, &u1, sizeof(f1));
        float out_vals[2] = {f0, f1};
        std::cout << "[QPSK_demod] Recovered: " << f0 << ", " << f1 << std::endl;

        pmt::pmt_t out_vec = pmt::init_f32vector(2, out_vals);
        message_port_pub(d_out_port, pmt::cons(pmt::PMT_NIL, out_vec));
      }
    }
  } /* namespace harmonia */
} /* namespace gr */
