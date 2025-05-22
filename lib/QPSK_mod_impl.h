/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_QPSK_MOD_IMPL_H
#define INCLUDED_HARMONIA_QPSK_MOD_IMPL_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/harmonia/QPSK_mod.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <iostream>
#include <cstring>
#include <cmath>

namespace gr
{
  namespace harmonia
  {

    class QPSK_mod_impl : public QPSK_mod
    {
    private:
      pmt::pmt_t d_in_port, d_out_port;
      int d_sps;
      double d_phase;
      void handle_msg(pmt::pmt_t msg);

    public:
      QPSK_mod_impl(int sps);
      ~QPSK_mod_impl() override;
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_QPSK_MOD_IMPL_H */
