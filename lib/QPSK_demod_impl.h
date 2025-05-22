/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_QPSK_DEMOD_IMPL_H
#define INCLUDED_HARMONIA_QPSK_DEMOD_IMPL_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/harmonia/QPSK_demod.h>
#include <gnuradio/harmonia/pmt_constants.h>

namespace gr
{
  namespace harmonia
  {

    class QPSK_demod_impl : public QPSK_demod
    {
    private:
      pmt::pmt_t d_in_port, d_out_port;
      int d_sps;
      std::deque<bool> d_bits;

      void handle_msg(pmt::pmt_t msg);

    public:
      QPSK_demod_impl(int sps);
      ~QPSK_demod_impl() override;
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_QPSK_DEMOD_IMPL_H */
